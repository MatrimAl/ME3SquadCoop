# Repo sahibi için kısa GitHub rehberi

## Issue ile PR arasındaki fark

Issue bir hata, fikir veya araştırma kaydıdır. Kod kendiliğinden değişmez. PR (pull request), bir branch'teki değişikliği başka bir branch'e alma teklifidir. Bir issue'yu kapatmak kodu birleştirmez; PR'daki Merge işlemi kodu birleştirir.

## Dışarıdan gelen katkıyı değerlendirme

1. Pull requests sekmesinden PR'ı aç. Hedef branch normalde dev/campaign-coop olmalı. Ana branch'e sürüm aktarımını sen yönetirsin.
2. Açıklama, ilgili issue ve yapılan testleri oku. Files changed bölümünden değişiklikleri incele. AI kullanılması tek başına doğruluk kanıtı değildir.
3. İlk katkı için Approve and run workflows görürsen önce workflow dosyalarını da incele. Bu düğme testlerin çalışmasına izin verir; kodu kabul etmez. CI yalnız GitHub'ın geçici makinelerinde çalışır; bilgisayarındaki oyunu çalıştırmaz.
4. Checks bölümünde Python tests ve Windows build başarılı olmalı. Bunlar gerçek oyun testi değildir. Oyun davranışını değiştiren PR için uygun izole oyun kanıtını da iste.
5. Düzeltme gerekiyorsa Files changed → Review changes → Request changes. Sadece soru soruyorsan Comment. Kabul ediyorsan Approve → Submit review.
6. Konuşmalar çözüldükten ve kontroller geçtikten sonra Squash and merge ile al. Bu işlem PR'ın değişikliklerini tek commit olarak birleştirir. Geliştirme branch'indeki kabul edilmiş değişiklikler daha sonra ayrı bir PR ile main'e taşınır.

Draft PR devam eden iştir; hazır olunca Ready for review seçilir. Yeşil test tek başına insan onayı sayılmaz. Onaydan sonra yeni commit gelirse eski onay yeniden istenir.

## Kendi PR'ların

GitHub kendi PR'ını Approve etmene izin vermez. Şu an tek maintainer olduğun için yönetici istisnası korunur. Kendi değişikliklerini önce sohbet içinde incelet, testlerin geçmesini bekle ve yalnız o zaman yönetici istisnasıyla birleştir. Dışarıdan gelen PR'larda normal bir onay ver. Otomatik onay veya otomatik merge kurulmadı.

## Issue düzeni

New issue ekranında hata, özellik isteği veya araştırma/görev formu seçilir. Formlar derleme/commit, tekrar adımları, beklenen-gözlenen sonuç, kanıt ve kapsam ister. Eksik bilgi varsa yorumla iste. Etiketler: bug, enhancement, research, help wanted, good first issue, needs-triage. İlk görev için good first issue etiketini ancak iş küçük ve sınırları netse kullan.

Hata doğrulanınca kapsamını netleştir; bir PR ile gerçekten çözüldüğünde kapat. Aynı sorun varsa eski issue'ya bağlantı verip duplicate olarak kapatabilirsin. Issue'ya yazılan çalışma niyeti başkalarının araştırmasına engel veya özel hak vermez.

## Koruma ve CI

main ve dev/campaign-coop için PR, bir onay, CODEOWNERS incelemesi, güncel branch üzerinde başarılı kontroller ve konuşmaların çözülmesi gerekir. Force-push ve branch silme kapalıdır. Yönetici istisnası tek maintainer nedeniyle korunur. Actions tokenı salt okuma izinlidir; Actions'ın PR onaylaması kapalıdır. Fork katkıları yazma tokenı veya repo secret'ı almaz. Otomatik dağıtım yoktur.

Settings değişiklikleri Git dosyası değildir. Bu rehber amaçlanan ayarları açıklar; gerçek ayarları Settings → Branches ve Actions altında görebilirsin.
