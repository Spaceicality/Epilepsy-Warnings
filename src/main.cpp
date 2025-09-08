#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/utils/web.hpp>
#include "json.hpp"

using namespace geode::prelude;

class $modify(MyLevelInfoLayer, LevelInfoLayer) {
    struct Fields {
        EventListener<web::WebTask> m_listener;
    };

public:
    bool init(GJGameLevel* level, bool p1) {
        if (!LevelInfoLayer::init(level, p1))
            return false;

        int levelID = level->m_levelID.value();

        // Only check if level ID is valid
        if (levelID < 1) {
            log::warn("Invalid level ID!");
            return true;
        }

        // URL to your hosted database.json
        auto url = "https://nolananderson.dev/database.json";

        // Setup web request listener
        m_fields->m_listener.bind([this, levelID](web::WebTask::Event* event) {
            if (auto* response = event->getValue()) {
                handleApiResponse(response, levelID);
            }
            else if (event->isCancelled()) {
                log::warn("Database request was cancelled");
            }
            else if (auto* progress = event->getProgress()) {
                // Optional: log download progress
            }
        });

        // Send the request
        auto request = web::WebRequest();
        m_fields->m_listener.setFilter(request.get(url));

        return true;
    }

private:
    void handleApiResponse(web::WebResponse* response, int levelID) {
        auto dbString = response->string().unwrapOr("{}");

        try {
            auto json = nlohmann::json::parse(dbString);

            // check if levelID is in the list
            bool flagged = false;
            for (auto& id : json) {
                if (id.is_number_integer() && id.get<int>() == levelID) {
                    flagged = true;
                    break;
                }
            }

            if (!flagged) {
                return;
            }

            // Run in main thread so the alert stays visible
            queueInMainThread([this] {
                auto warningAlert = FLAlertLayer::create(
                    "WARNING!",
                    "This level is in the Epileptic Warnings Database and may contain seizure-inducing effects!",
                    "OK"
                );

                if (warningAlert) {
                    FMODAudioEngine::sharedEngine()->playEffect("chestClick.ogg");

                    // Attach to *this* LevelInfoLayer
                    this->addChild(warningAlert, 1000);
                }
            });
        }
        catch (std::exception const& e) {
            log::error("Failed to parse database.json: {}", e.what());
        }
    }
};

