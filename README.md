
🚀 Smart Attendance System (IoT & AI Based)
Bu proje, eğitim kurumlarındaki manuel yoklama süreçlerini otomatikleştirmek ve dijitalleştirmek amacıyla geliştirilmiş bir "Smart Campus" çözümüdür. ESP32-CAM donanımı ile Python tabanlı yapay zeka algoritmalarını birleştirerek uçtan uca bir sistem sunar.

🛠️ Teknik Özellikler & Akış
Görüntü İşleme: ESP32-CAM üzerinden alınan anlık görüntüler, OpenCV ve face_recognition kütüphanesi kullanılarak işlenir.

Bulut Entegrasyonu: Tanımlanan öğrencilerin katılım bilgileri, Google Sheets API aracılığıyla anlık olarak bulut tabanlı bir tabloya kaydedilir.

Otomatik Bildirim: Yoklama işlemi başarıyla tamamlandığında, SMTP protokolü kullanılarak ilgili kişilere bilgilendirme e-postası gönderilir.

Donanım Kontrolü: ESP32-CAM ve Python arasındaki veri akışı HTTP/IP protokolü üzerinden sağlanır.

💻 Kullanılan Teknolojiler
Programlama Dilleri: Python, C++ (Arduino IDE).

Kütüphaneler: OpenCV, face_recognition, gspread (Google Sheets API), Smtplib.

Donanım: ESP32-CAM Modülü, FTDI Programlayıcı.

Araçlar: Arduino IDE, PyCharm/VS Code.
