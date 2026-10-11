#include "ota.h"

#include <Update.h>

#include "pin_http.h"

namespace {

// ESP image header: magic byte, then the target chip id at offset 12.
constexpr uint8_t kImageMagic = 0xE9;
constexpr size_t kChipIdOffset = 12;

uint16_t chipId(cwc::Chip chip) {
  switch (chip) {
    case cwc::Chip::Esp32S3: return 0x0009;
    case cwc::Chip::Esp32C3: return 0x0005;
    case cwc::Chip::Esp32: break;
  }
  return 0x0000;
}

// One upload at a time; the network task runs every handler, so no locking.
struct Upload {
  bool active = false;
  const char* error = nullptr;  // set when the upload is refused or fails
};
Upload upload;

std::string bearer(AsyncWebServerRequest* request) {
  if (!request->hasHeader("Authorization")) return "";
  String value = request->header("Authorization");
  return value.startsWith("Bearer ") ? value.substring(7).c_str() : "";
}

}  // namespace

void registerOta(AsyncWebServer& server, auth::PinGuard& pin, cwc::Chip chip, std::function<void()> onStart,
                 std::function<void()> onDone) {
  server.on(
      "/api/update", HTTP_POST,
      // Runs once the whole body has been received.
      [onDone](AsyncWebServerRequest* request) {
        const char* error = upload.error;
        upload = Upload{};
        if (error == nullptr && !Update.hasError()) {
          request->send(200, "application/json", "{\"ok\":true,\"restart\":true}");
          onDone();
          return;
        }
        int code = error != nullptr && strcmp(error, "pin_required") == 0 ? 401 : 400;
        String body = String("{\"error\":\"") + (error ? error : "update_failed") + "\",\"detail\":\"" +
                      Update.errorString() + "\"}";
        request->send(code, "application/json", body);
      },
      // Runs for each chunk of the uploaded file.
      [&pin, chip, onStart](AsyncWebServerRequest* request, const String&, size_t index, uint8_t* data, size_t len,
                            bool final) {
        if (index == 0) {
          upload = Upload{};
          if (!pin.authorized(bearer(request), millis())) {
            upload.error = "pin_required";
          } else if (len <= kChipIdOffset + 1 || data[0] != kImageMagic) {
            upload.error = "not_firmware";
          } else if ((data[kChipIdOffset] | data[kChipIdOffset + 1] << 8) != chipId(chip)) {
            upload.error = "wrong_chip";
          } else if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
            upload.error = "update_failed";
          } else {
            upload.active = true;
            Serial.println("[update] receiving firmware");
            onStart();
          }
        }
        if (!upload.active || upload.error != nullptr) return;

        if (Update.write(data, len) != len) {
          upload.error = "update_failed";
          Update.abort();
          upload.active = false;
          return;
        }
        if (final) {
          // Verifies the image and only then marks the new slot bootable.
          if (!Update.end(true)) upload.error = "update_failed";
          upload.active = false;
          Serial.printf("[update] %s (%u bytes)\n", upload.error ? "failed" : "done", unsigned(index + len));
        }
      });
}
