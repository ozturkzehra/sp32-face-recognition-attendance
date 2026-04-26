import requests
import cv2
import numpy as np
import face_recognition
import os
import time

# ================= ESP32 =================
ESP32_URL = "http://172.20.10.2/image"

# ================= GOOGLE SHEETS =================
SHEET_URL = "https://script.google.com/macros/s/AKfycbzO8kaxUaHULmDWUYRYmI4ZCqiyXUxK1ij-uV-cKFsM2IXnBQpwJn5Jf2yw0i6CCFoc/exec"

# ================= YÜZ VERİLERİ =================
known_encodings = []
known_names = []

for file in os.listdir("faces"):
    if file.lower().endswith((".jpg", ".jpeg", ".png")):
        path = os.path.join("faces", file)
        image = face_recognition.load_image_file(path)
        encodings = face_recognition.face_encodings(image)

        if len(encodings) == 1:
            known_encodings.append(encodings[0])
            known_names.append(os.path.splitext(file)[0].lower())

print("✅ Yüklü yüzler:", known_names)

if len(known_encodings) == 0:
    print("❌ faces klasöründe geçerli yüz yok")
    exit()

# ================= OPENCV =================
cv2.namedWindow("ESP32 FACE", cv2.WINDOW_NORMAL)

last_sent_name = ""
last_sent_time = 0

# ================= ANA DÖNGÜ =================
while True:
    # ---------- ESP32'dan görüntü al ----------
    try:
        r = requests.get(ESP32_URL, timeout=3)
    except:
        print("❌ ESP32 bağlantı yok")
        time.sleep(1)
        continue

    frame = cv2.imdecode(np.frombuffer(r.content, np.uint8), cv2.IMREAD_COLOR)
    if frame is None:
        continue

    # ---------- Yüz tanıma ----------
    rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    face_locations = face_recognition.face_locations(rgb, model="hog")
    face_encodings = face_recognition.face_encodings(rgb, face_locations)

    detected_name = ""

    for (top, right, bottom, left), face_encoding in zip(face_locations, face_encodings):
        distances = face_recognition.face_distance(known_encodings, face_encoding)
        min_distance = np.min(distances)

        if min_distance < 0.45:
            detected_name = known_names[np.argmin(distances)]

        label = detected_name if detected_name else "Bilinmiyor"

        cv2.rectangle(frame, (left, top), (right, bottom), (0, 255, 0), 2)
        cv2.putText(frame, label, (left, top - 10),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.9, (0, 255, 0), 2)

    # ---------- GOOGLE SHEETS'E GÖNDER ----------
    now = time.time()
    if detected_name:
        if detected_name != last_sent_name or now - last_sent_time > 15:
            try:
                requests.get(SHEET_URL + "?ogrenci=" + detected_name)
                print("📤 Sheets'e yazıldı:", detected_name)
                last_sent_name = detected_name
                last_sent_time = now
            except:
                print("❌ Sheets gönderme hatası")

    # ---------- Görüntü ----------
    cv2.imshow("ESP32 FACE", frame)

    if cv2.waitKey(1) & 0xFF == 27:
        break

cv2.destroyAllWindows()