#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

class $modify(MyLevelInfoLayer, LevelInfoLayer) {
    struct Fields {
        EventListener<web::WebTask> m_listener;
    };

public:
    bool init(GJGameLevel* level, bool p1) {
        if (!LevelInfoLayer::init(level, p1)) 
            return false;

        int levelID = level->m_levelID.value(); // convert m_levelID to human-readable/human-expected level ID value for the API, otherwise the API will return incorrect results

        // Only check if level ID is valid
        if (levelID < 1) {
            log::warn("Invalid level ID!");
            return true;
        }

        // Construct API URL
        auto url = fmt::format(
            "https://epilepsywarningapi.fluxitegmd.workers.dev/id/{}", 
            levelID
        );
        // log::info("Checking epilepsy warning for level {}", levelID);

        // Setup web request listener
        m_fields->m_listener.bind([this, levelID](web::WebTask::Event* event) {
            if (auto* response = event->getValue()) {
                handleApiResponse(response);
            }
            else if (event->isCancelled()) {
                log::warn("API request for {} was cancelled", levelID);
            }
            else if (auto* progress = event->getProgress()) {
                // Optional: Handle progress updates if needed
                // log::info("Request progress: {:.0f}%", progress->downloadProgress().value_or(0.f) * 100);
            }
        });

        // Send the request
        auto request = web::WebRequest();
        m_fields->m_listener.setFilter(request.get(url));

        return true;
    }

private:
    void handleApiResponse(web::WebResponse* response) {
        auto result = response->string().unwrapOr("false");
        
        if (result != "true") {
            return;
        }

        queueInMainThread([this] {
            auto warningAlert = FLAlertLayer::create(
                "WARNING!",
                "This level is in the Epileptic Warnings Database and may contain seizure-inducing effects!",
                "OK"
            );

            if (warningAlert) {
                FMODAudioEngine::sharedEngine()->playEffect("chestClick.ogg");
                CCDirector::sharedDirector()->getRunningScene()->addChild(warningAlert, 1000);
            }
        });
    }
};



