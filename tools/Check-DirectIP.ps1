<# Read-only local interface/route/UDP diagnostic. Sends no peer traffic. #>
[CmdletBinding()]
param(
    [ValidateSet('host', 'client')][string]$Role = 'host',
    [string]$BindIPv4,
    [string]$PeerIPv4,
    [ValidateRange(1, 65535)][int]$Port = 17779,
    [switch]$LoopbackQA,
    [switch]$ShowAddresses
)
$ErrorActionPreference = 'Stop'

function ConvertTo-StrictIPv4([string]$Text) {
    if ($Text -cnotmatch '^(?:0|[1-9][0-9]{0,2})(?:\.(?:0|[1-9][0-9]{0,2})){3}$') {
        throw 'Expected canonical dotted-decimal IPv4 literal.'
    }
    $address = $null
    if (-not [System.Net.IPAddress]::TryParse($Text, [ref]$address) -or
        $address.AddressFamily -ne [System.Net.Sockets.AddressFamily]::InterNetwork -or
        $address.ToString() -cne $Text) {
        throw 'Invalid IPv4 address.'
    }
    $bytes = $address.GetAddressBytes()
    if ($bytes[0] -eq 0 -or $bytes[0] -ge 224 -or ($bytes[0] -eq 169 -and $bytes[1] -eq 254)) {
        throw 'Expected usable unicast IPv4 address.'
    }
    if ([System.Net.IPAddress]::IsLoopback($address) -and -not $LoopbackQA) {
        throw 'Loopback requires explicit -LoopbackQA.'
    }
    if ($LoopbackQA -and -not [System.Net.IPAddress]::IsLoopback($address)) {
        throw 'Loopback QA must remain entirely on loopback.'
    }
    return $address
}

function Get-SubnetAddress([System.Net.IPAddress]$Address, [System.Net.IPAddress]$Mask) {
    $addressBytes = $Address.GetAddressBytes()
    $maskBytes = $Mask.GetAddressBytes()
    $networkBytes = New-Object byte[] 4
    for ($index = 0; $index -lt 4; $index++) {
        $networkBytes[$index] = $addressBytes[$index] -band $maskBytes[$index]
    }
    return [System.Net.IPAddress]::new($networkBytes)
}

try {
    $interfaces = @([System.Net.NetworkInformation.NetworkInterface]::GetAllNetworkInterfaces())
    $radmin = @($interfaces | Where-Object { $_.Name -match 'Radmin|Famatech' -or $_.Description -match 'Radmin|Famatech' })
    $radminUp = @($radmin | Where-Object { $_.OperationalStatus -eq [System.Net.NetworkInformation.OperationalStatus]::Up })
    $radminIPv4 = @(
        foreach ($adapter in $radminUp) {
            foreach ($unicast in $adapter.GetIPProperties().UnicastAddresses) {
                if ($unicast.Address.AddressFamily -eq [System.Net.Sockets.AddressFamily]::InterNetwork) {
                    $bytes = $unicast.Address.GetAddressBytes()
                    if ($bytes[0] -gt 0 -and $bytes[0] -lt 224 -and $bytes[0] -ne 127 -and
                        -not ($bytes[0] -eq 169 -and $bytes[1] -eq 254)) {
                        [pscustomobject]@{ Address = $unicast.Address; Mask = $unicast.IPv4Mask }
                    }
                }
            }
        }
    )
    $allLocalIPv4 = @(
        foreach ($adapter in $interfaces) {
            if ($adapter.OperationalStatus -eq [System.Net.NetworkInformation.OperationalStatus]::Up) {
                foreach ($unicast in $adapter.GetIPProperties().UnicastAddresses) {
                    if ($unicast.Address.AddressFamily -eq [System.Net.Sockets.AddressFamily]::InterNetwork) {
                        $unicast.Address.ToString()
                    }
                }
            }
        }
    )
    $chosenBind = $null
    if ($BindIPv4) {
        $chosenBind = ConvertTo-StrictIPv4 $BindIPv4
    } elseif ($LoopbackQA) {
        $chosenBind = [System.Net.IPAddress]::Loopback
    } elseif ($radminIPv4.Count -eq 1) {
        $chosenBind = $radminIPv4[0].Address
    }
    $peer = $null
    if ($PeerIPv4) { $peer = ConvertTo-StrictIPv4 $PeerIPv4 }
    if ($Role -eq 'host' -and $peer) { throw 'Host diagnostic does not take a peer address.' }
    if ($peer) {
        foreach ($local in $radminIPv4) {
            $network = Get-SubnetAddress $local.Address $local.Mask
            if ((Get-SubnetAddress $peer $local.Mask).Equals($network)) {
                $peerBytes = $peer.GetAddressBytes()
                $maskBytes = $local.Mask.GetAddressBytes()
                $broadcastBytes = New-Object byte[] 4
                $networkBytes = $network.GetAddressBytes()
                $hostBits = 0
                for ($index = 0; $index -lt 4; $index++) {
                    $broadcastBytes[$index] = $networkBytes[$index] -bor (255 -bxor $maskBytes[$index])
                    $hostByte = 255 -bxor $maskBytes[$index]
                    for ($bit = 0; $bit -lt 8; $bit++) { if ($hostByte -band (1 -shl $bit)) { $hostBits++ } }
                }
                $broadcast = [System.Net.IPAddress]::new($broadcastBytes)
                if ($hostBits -ge 2 -and ($peer.Equals($network) -or $peer.Equals($broadcast))) {
                    throw 'Peer is a network/directed-broadcast address on the local VPN subnet.'
                }
            }
        }
    }
    $bindAssigned = $null -ne $chosenBind -and $allLocalIPv4 -contains $chosenBind.ToString()
    $bindOnRadmin = $null -ne $chosenBind -and @($radminIPv4 | Where-Object { $_.Address.Equals($chosenBind) }).Count -gt 0
    $routeAvailable = $false
    $routeObserved = $false
    if (-not $LoopbackQA -and $radminIPv4.Count -gt 0) {
        # CIM is unnecessary here. Numeric columns from route.exe are sufficient
        # and the variable endpoint never becomes executable shell text.
        $routeCommand = Get-Command route.exe -CommandType Application -ErrorAction SilentlyContinue
        if ($routeCommand) {
            $routeLines = @(& $routeCommand.Source print -4 2>$null)
            $routeAvailable = $LASTEXITCODE -eq 0
            if ($routeAvailable) {
                foreach ($local in $radminIPv4) {
                    $network = [regex]::Escape((Get-SubnetAddress $local.Address $local.Mask).ToString())
                    $mask = [regex]::Escape($local.Mask.ToString())
                    $localText = [regex]::Escape($local.Address.ToString())
                    if (@($routeLines | Where-Object { $_ -match "^\s*$network\s+$mask\s+.*\s+$localText\s+\d+\s*$" }).Count -gt 0) {
                        $routeObserved = $true
                    }
                }
            }
        }
    }
    $udpAvailable = $false
    $listenerKinds = @()
    try {
        $listeners = @([System.Net.NetworkInformation.IPGlobalProperties]::GetIPGlobalProperties().GetActiveUdpListeners() |
            Where-Object { $_.Port -eq $Port -and $_.Address.AddressFamily -eq [System.Net.Sockets.AddressFamily]::InterNetwork })
        $udpAvailable = $true
        $listenerKinds = @(
            foreach ($listener in $listeners) {
                if ($listener.Address.Equals([System.Net.IPAddress]::Any)) { 'wildcard' }
                elseif ([System.Net.IPAddress]::IsLoopback($listener.Address)) { 'loopback' }
                elseif (@($radminIPv4 | Where-Object { $_.Address.Equals($listener.Address) }).Count -gt 0) { 'radmin' }
                else { 'other-interface' }
            }
        ) | Select-Object -Unique
    } catch { $udpAvailable = $false }
    $localReady = $bindAssigned -and ($LoopbackQA -or ($bindOnRadmin -and $routeObserved))
    $result = [ordered]@{
        schema = 1; role = $Role; port = $Port; loopback_qa = [bool]$LoopbackQA
        radmin_adapter_present = $radmin.Count -gt 0; radmin_adapter_up = $radminUp.Count -gt 0
        radmin_usable_ipv4_count = $radminIPv4.Count; bind_address_assigned = [bool]$bindAssigned
        bind_on_radmin = [bool]$bindOnRadmin; local_interface_ready = [bool]$localReady
        route_table_read = [bool]$routeAvailable; vpn_subnet_route_observed = [bool]$routeObserved
        peer_configured = $null -ne $peer; udp_listener_table_read = [bool]$udpAvailable
        udp_listener_kinds_on_port = @($listenerKinds)
        peer_reachability_tested = $false; udp_transport_tested = $false
        firewall_changed = $false; game_launched = $false; campaign_coop_verified = $false
    }
    if ($ShowAddresses) {
        $result.radmin_ipv4 = @($radminIPv4 | ForEach-Object { $_.Address.ToString() })
        $result.bind_ipv4 = if ($chosenBind) { $chosenBind.ToString() } else { $null }
        $result.peer_ipv4 = if ($peer) { $peer.ToString() } else { $null }
    }
    $result | ConvertTo-Json -Depth 4
    if (-not $localReady) { exit 1 }
} catch {
    # Default output does not expose endpoints, usernames or paths.
    [ordered]@{ schema = 1; diagnostic_completed = $false; error_type = $_.Exception.GetType().Name
        peer_reachability_tested = $false; udp_transport_tested = $false; campaign_coop_verified = $false } | ConvertTo-Json
    exit 2
}
