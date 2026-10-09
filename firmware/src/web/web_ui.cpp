#include "web/web_ui.h"

#include "web/assets.h"

namespace {

const WebAsset* findAsset(const char* path) {
  for (const WebAsset& asset : kWebAssets) {
    if (strcmp(asset.path, path) == 0) return &asset;
  }
  return nullptr;
}

void sendAsset(AsyncWebServerRequest* request, const WebAsset& asset) {
  AsyncWebServerResponse* response = request->beginResponse(200, asset.type, asset.data, asset.size);
  response->addHeader("Content-Encoding", "gzip");
  // Assets change only with the firmware; let the browser revalidate each time.
  response->addHeader("Cache-Control", "no-cache");
  request->send(response);
}

}  // namespace

void registerWebUi(AsyncWebServer& server, std::function<bool()> inSetup) {
  const WebAsset* index = findAsset("/index.html");
  const WebAsset* setup = findAsset("/setup.html");
  server.on("/", HTTP_GET, [=](AsyncWebServerRequest* request) {
    sendAsset(request, *(inSetup() ? setup : index));
  });

  for (const WebAsset& asset : kWebAssets) {
    const WebAsset* current = &asset;
    server.on(asset.path, HTTP_GET, [current](AsyncWebServerRequest* request) { sendAsset(request, *current); });
  }
}
