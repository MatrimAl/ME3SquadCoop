#ifndef ME3_SQUAD_COOP_DIRECT_IP_NATIVE_H
#define ME3_SQUAD_COOP_DIRECT_IP_NATIVE_H

/* Endpoint preparation only: no sockets, game calls, activation changes or hooks.
 * Call load_environment and require DIRECT_IP_NATIVE_OK before launch/hook setup.
 * Python validation is supplementary; these checks also cover native-only entry.
 */
#include <windows.h>
#include <stddef.h>
#include <string.h>
#include <wchar.h>

typedef struct DirectIPNativeConfig {
    wchar_t bind_ipv4[16];
    wchar_t peer_ipv4[16]; /* empty for an explicitly configured host */
    unsigned short port;
    int loopback_qa;
    int defaults_used;
} DirectIPNativeConfig;

typedef enum DirectIPNativeStatus {
    DIRECT_IP_NATIVE_OK = 0,
    DIRECT_IP_NATIVE_BAD_ARGUMENT = 1,
    DIRECT_IP_NATIVE_INCOMPLETE = 2,
    DIRECT_IP_NATIVE_BAD_ADDRESS = 3,
    DIRECT_IP_NATIVE_BAD_PORT = 4,
    DIRECT_IP_NATIVE_BAD_QA_FLAG = 5,
    DIRECT_IP_NATIVE_ENVIRONMENT_ERROR = 6,
    DIRECT_IP_NATIVE_HOST_HAS_PEER = 7,
    DIRECT_IP_NATIVE_QA_HAS_REMOTE_ADDRESS = 8
} DirectIPNativeStatus;

/* Returns 1 only for a canonical, usable unicast literal. Public unicast such as
 * the owner's observed Radmin 26/8 is supported. A subnet-directed broadcast can
 * only be rejected with the actual prefix; Python config handles that optional
 * prefix check. Do not mistake every address ending in .255 for a broadcast.
 * octets may be NULL when only validation is wanted.
 */
static int direct_ip_native_parse_ipv4(const wchar_t *text, int loopback_qa,
                                       unsigned char octets[4]) {
    unsigned char values[4];
    const wchar_t *cursor = text;
    unsigned int part;
    if (!text || (loopback_qa != 0 && loopback_qa != 1)) return 0;
    for (part = 0; part < 4; ++part) {
        unsigned int value = 0, digits = 0;
        if (*cursor < L'0' || *cursor > L'9') return 0;
        if (*cursor == L'0' && cursor[1] >= L'0' && cursor[1] <= L'9') return 0;
        while (*cursor >= L'0' && *cursor <= L'9') {
            if (++digits > 3) return 0;
            value = value * 10u + (unsigned int)(*cursor++ - L'0');
            if (value > 255u) return 0;
        }
        values[part] = (unsigned char)value;
        if (part < 3) {
            if (*cursor++ != L'.') return 0;
        } else if (*cursor != L'\0') return 0;
    }
    if (values[0] == 0 || values[0] >= 224 ||
        (values[0] == 169 && values[1] == 254) ||
        (values[0] == 127 && !loopback_qa)) return 0;
    if (octets) memcpy(octets, values, sizeof values);
    return 1;
}

static int direct_ip_native_parse_port(const wchar_t *text, unsigned short *port) {
    unsigned int value = 0, digits = 0;
    const wchar_t *cursor = text;
    if (!text || !port || *cursor < L'1' || *cursor > L'9') return 0;
    while (*cursor >= L'0' && *cursor <= L'9') {
        if (++digits > 5) return 0;
        value = value * 10u + (unsigned int)(*cursor++ - L'0');
        if (value > 65535u) return 0;
    }
    if (*cursor != L'\0' || value == 0) return 0;
    *port = (unsigned short)value;
    return 1;
}

/* Input is already validated (maximum IPv4 text length = 15). */
static void direct_ip_native_copy_ipv4(wchar_t destination[16], const wchar_t *source) {
    size_t index = 0;
    do { destination[index] = source[index]; } while (source[index++] != L'\0');
}

/* Pure parser: NULL means unset, L"" means explicitly present but empty.
 * LOOPBACK_QA has exactly the values L"0" or L"1". Defaults are allowed ONLY
 * when all four inputs are NULL. Explicit host configs must omit peer, matching
 * DirectIPConfig. Explicit client configs must provide all four inputs.
 */
static DirectIPNativeStatus direct_ip_native_parse_config(
    DirectIPNativeConfig *out, int is_client,
    const wchar_t *bind_ipv4, const wchar_t *peer_ipv4,
    const wchar_t *port_text, const wchar_t *qa_text) {
    DirectIPNativeConfig config;
    unsigned char bind_octets[4], peer_octets[4] = {0, 0, 0, 0};
    if (!out) return DIRECT_IP_NATIVE_BAD_ARGUMENT;
    memset(out, 0, sizeof *out); /* failed parse must not leave usable stale config */
    memset(&config, 0, sizeof config);
    if (is_client != 0 && is_client != 1) return DIRECT_IP_NATIVE_BAD_ARGUMENT;
    if (!bind_ipv4 && !peer_ipv4 && !port_text && !qa_text) {
        direct_ip_native_copy_ipv4(config.bind_ipv4, L"127.0.0.1");
        if (is_client) direct_ip_native_copy_ipv4(config.peer_ipv4, L"127.0.0.1");
        config.port = 17779;
        config.loopback_qa = 1;
        config.defaults_used = 1;
        *out = config;
        return DIRECT_IP_NATIVE_OK;
    }
    if (!bind_ipv4 || !port_text || !qa_text || (is_client && !peer_ipv4))
        return DIRECT_IP_NATIVE_INCOMPLETE;
    if (!is_client && peer_ipv4) return DIRECT_IP_NATIVE_HOST_HAS_PEER;
    if ((qa_text[0] != L'0' && qa_text[0] != L'1') || qa_text[1] != L'\0')
        return DIRECT_IP_NATIVE_BAD_QA_FLAG;
    config.loopback_qa = qa_text[0] == L'1';
    if (!direct_ip_native_parse_port(port_text, &config.port)) return DIRECT_IP_NATIVE_BAD_PORT;
    if (!direct_ip_native_parse_ipv4(bind_ipv4, config.loopback_qa, bind_octets) ||
        (is_client && !direct_ip_native_parse_ipv4(peer_ipv4, config.loopback_qa, peer_octets)))
        return DIRECT_IP_NATIVE_BAD_ADDRESS;
    if (config.loopback_qa && (bind_octets[0] != 127 || (is_client && peer_octets[0] != 127)))
        return DIRECT_IP_NATIVE_QA_HAS_REMOTE_ADDRESS;
    direct_ip_native_copy_ipv4(config.bind_ipv4, bind_ipv4);
    if (is_client) direct_ip_native_copy_ipv4(config.peer_ipv4, peer_ipv4);
    *out = config;
    return DIRECT_IP_NATIVE_OK;
}

/* Distinguishes absent from empty and fails on truncation; no NetAPI dependency.
 * Return 1 for a successful read (present or absent), 0 for any read error.
 */
static int direct_ip_native_read_environment(const wchar_t *name, wchar_t *buffer,
                                            DWORD capacity, int *present) {
    DWORD count, error;
    if (!name || !buffer || !capacity || !present) return 0;
    buffer[0] = L'\0';
    SetLastError(ERROR_SUCCESS);
    count = GetEnvironmentVariableW(name, buffer, capacity);
    error = GetLastError();
    if (count >= capacity) return 0;
    if (count == 0 && error == ERROR_ENVVAR_NOT_FOUND) {
        *present = 0;
        return 1;
    }
    if (count == 0 && error != ERROR_SUCCESS) return 0;
    *present = 1;
    return 1;
}

static DirectIPNativeStatus direct_ip_native_load_environment(DirectIPNativeConfig *out,
                                                              int is_client) {
    wchar_t bind_ipv4[16], peer_ipv4[16], port_text[6], qa_text[2];
    int bind_present = 0, peer_present = 0, port_present = 0, qa_present = 0;
    if (!out) return DIRECT_IP_NATIVE_BAD_ARGUMENT;
    memset(out, 0, sizeof *out);
    if (!direct_ip_native_read_environment(L"ME3_SQUAD_COOP_BIND_IPV4", bind_ipv4, 16, &bind_present) ||
        !direct_ip_native_read_environment(L"ME3_SQUAD_COOP_PEER_IPV4", peer_ipv4, 16, &peer_present) ||
        !direct_ip_native_read_environment(L"ME3_SQUAD_COOP_PORT", port_text, 6, &port_present) ||
        !direct_ip_native_read_environment(L"ME3_SQUAD_COOP_LOOPBACK_QA", qa_text, 2, &qa_present))
        return DIRECT_IP_NATIVE_ENVIRONMENT_ERROR;
    return direct_ip_native_parse_config(out, is_client,
        bind_present ? bind_ipv4 : NULL, peer_present ? peer_ipv4 : NULL,
        port_present ? port_text : NULL, qa_present ? qa_text : NULL);
}

/* Pure deterministic harness entry: no environment mutation or socket creation.
 * Caller may export or invoke this after a normal build; 1 means all checks pass.
 * Deliberately not executed during header authoring while a game test is active.
 */
static int direct_ip_native_selftest(void) {
    static const wchar_t *bad_addresses[] = {
        L"0.0.0.0", L"0.1.2.3", L"169.254.1.2", L"224.0.0.1", L"240.1.2.3", L"255.255.255.255",
        L"26.1.2.3?listen", L"26.1.2.3:17779", L"26.1.2.3;quit", L"26.1.2.3\n", L" 26.1.2.3",
        L"026.1.2.3", L"26.1.2", L"26.1.2.256", L"26..2.3", L"localhost", L"::1", L"\"26.1.2.3\""
    };
    static const wchar_t *bad_ports[] = {L"", L"0", L"65536", L"017779", L"17779\n", L"17779?listen", L"-1"};
    DirectIPNativeConfig config;
    unsigned short port = 0;
    size_t index;
    for (index = 0; index < sizeof bad_addresses / sizeof bad_addresses[0]; ++index)
        if (direct_ip_native_parse_ipv4(bad_addresses[index], 0, NULL)) return 0;
    for (index = 0; index < sizeof bad_ports / sizeof bad_ports[0]; ++index)
        if (direct_ip_native_parse_port(bad_ports[index], &port)) return 0;
    if (!direct_ip_native_parse_ipv4(L"26.123.45.67", 0, NULL) ||
        direct_ip_native_parse_ipv4(L"127.0.0.1", 0, NULL) ||
        !direct_ip_native_parse_ipv4(L"127.0.0.1", 1, NULL)) return 0;
    if (!direct_ip_native_parse_port(L"65535", &port) || port != 65535) return 0;
    if (direct_ip_native_parse_config(&config, 1, NULL, NULL, NULL, NULL) != DIRECT_IP_NATIVE_OK ||
        !config.defaults_used || !config.loopback_qa || config.port != 17779 ||
        wcscmp(config.peer_ipv4, L"127.0.0.1") != 0) return 0;
    if (direct_ip_native_parse_config(&config, 1, L"26.1.2.3", L"26.2.3.4", L"17779", L"0") != DIRECT_IP_NATIVE_OK ||
        config.defaults_used || config.loopback_qa || wcscmp(config.bind_ipv4, L"26.1.2.3") != 0) return 0;
    if (direct_ip_native_parse_config(&config, 0, L"26.1.2.3", NULL, L"17779", L"0") != DIRECT_IP_NATIVE_OK) return 0;
    if (direct_ip_native_parse_config(&config, 1, L"127.0.0.1", L"127.0.0.1", L"17779", L"1") != DIRECT_IP_NATIVE_OK) return 0;
    if (direct_ip_native_parse_config(&config, 1, L"26.1.2.3", NULL, L"17779", L"0") != DIRECT_IP_NATIVE_INCOMPLETE) return 0;
    if (direct_ip_native_parse_config(&config, 0, NULL, NULL, L"17779", NULL) != DIRECT_IP_NATIVE_INCOMPLETE) return 0;
    if (direct_ip_native_parse_config(&config, 0, L"", NULL, L"17779", L"0") != DIRECT_IP_NATIVE_BAD_ADDRESS || config.port != 0) return 0;
    if (direct_ip_native_parse_config(&config, 1, L"26.1.2.3", L"26.2.3.4", L"17779", L"1") != DIRECT_IP_NATIVE_QA_HAS_REMOTE_ADDRESS) return 0;
    if (direct_ip_native_parse_config(&config, 0, L"26.1.2.3", L"26.2.3.4", L"17779", L"0") != DIRECT_IP_NATIVE_HOST_HAS_PEER) return 0;
    if (direct_ip_native_parse_config(&config, 0, L"26.1.2.3", NULL, L"17779", L"true") != DIRECT_IP_NATIVE_BAD_QA_FLAG) return 0;
    return 1;
}

#endif /* ME3_SQUAD_COOP_DIRECT_IP_NATIVE_H */
