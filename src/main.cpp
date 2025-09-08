#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/ui/Notification.hpp>
#include <Geode/loader/SettingV3.hpp>
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
        if (levelID < 1) {
            log::warn("Invalid level ID!");
            return true;
        }

        // Get the toggle from mod settings
        bool useGeodeNotification = Mod::get()->getSettingValue<bool>("use-geode-notification");

        auto url = "https://nolananderson.dev/Epilepsy-Warnings/database.json";
        log::info("Fetching database.json from {}", url);

        m_fields->m_listener.bind([this, levelID, useGeodeNotification](web::WebTask::Event* event) {
            if (auto* response = event->getValue()) {
                auto dbString = response->string().unwrapOr("[]");
                log::info("Fetched database.json: {}", dbString);

                try {
                    auto json = nlohmann::json::parse(dbString);

                    bool flagged = false;
                    for (auto& entry : json) {
                        if (entry.contains("id") && entry["id"].is_number_integer() &&
                            entry["id"].get<int>() == levelID) {
                            flagged = true;
                            break;
                        }
                    }

                    if (!flagged) {
                        log::info("Level {} is not flagged", levelID);
                        return;
                    }

                    log::info("Level {} is flagged, showing alert", levelID);

                    queueInMainThread([this, useGeodeNotification] {
                        if (useGeodeNotification) {
                            // Show Geode notification
                            geode::Notification::create(
                                "Epilepsy Warning!",
                                geode::NotificationIcon::Warning,
                                2.5f
                            )->show();
                        } else {
                            // Show FLAlertLayer
                            auto warningAlert = FLAlertLayer::create(
                                "WARNING!",
                                "This level is in the Epileptic Warnings Database and may contain seizure-inducing effects!",
                                "OK"
                            );

                            if (warningAlert) {
                                FMODAudioEngine::sharedEngine()->playEffect("chestClick.ogg");
                                this->addChild(warningAlert, 1000);
                            }
                        }
                    });

                } catch (std::exception const& e) {
                    log::error("Failed to parse database.json: {}", e.what());
                }

            } else if (event->isCancelled()) {
                log::warn("Database request was cancelled");
            }
        });

        auto request = web::WebRequest();
        m_fields->m_listener.setFilter(request.get(url));

        return true;
    }
};
