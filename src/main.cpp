#include <Geode/Geode.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/utils/web.hpp>

using namespace geode::prelude;

class $modify(MyLevelInfoLayer, LevelInfoLayer) {
    struct Fields {
        EventListener<web::WebTask> m_listener;
        // Store alert reference to prevent premature deletion
        FLAlertLayer* m_warningAlert = nullptr;
    };

public:
    bool init(GJGameLevel* level, bool p1) {
        if (!LevelInfoLayer::init(level, p1)) 
            return false;

        // Only check if level ID is valid
        if (level->m_levelID < 1) {
            log::warn("Invalid level ID!");
            return true;
        }

        // Construct API URL
        auto url = fmt::format(
            "https://epilepsywarningapi.fluxitegmd.workers.dev/id/{}", 
            level->m_levelID
        );
        log::info("Checking epilepsy warning for level {}", level->m_levelID);

        // Setup web request listener
        m_fields->m_listener.bind([this](web::WebTask::Event* event) {
            if (auto* response = event->getValue()) {
                handleApiResponse(response);
            }
            else if (event->isCancelled()) {
                log::warn("API request was cancelled");
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
        
        // Create and store warning alert
        m_fields->m_warningAlert = FLAlertLayer::create(
            "WARNING!", 
            "This level is in the Epileptic Warnings Database and may contain seizure-inducing effects!",
            "OK"
        );

        if (m_fields->m_warningAlert) {
            m_fields->m_warningAlert->m_scene = this; // Set the scene to prevent fade-out
            FMODAudioEngine::sharedEngine()->playEffect("chestClick.ogg");
            m_fields->m_warningAlert->show();
        }
    }
};