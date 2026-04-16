#include "ui_page_registry.h"

#include <esp_log.h>

namespace {

const char* const TAG = "UiPageRegistry";

const char* UiPageIdName(UiPageId id) {
    switch (id) {
        case UiPageId::FactoryTest:
            return "FactoryTest";
        case UiPageId::Vocabulary:
            return "Vocabulary";
        default:
            return "Unknown";
    }
}

}  // namespace

bool UiPageRegistry::Register(std::unique_ptr<IUiPage> page) {
    if (!page) {
        ESP_LOGW(TAG, "UI_PAGE register result=fail reason=null_page");
        return false;
    }

    const UiPageId id = page->Id();
    if (Get(id) != nullptr) {
        ESP_LOGW(TAG, "UI_PAGE register id=%u name=%s result=fail reason=duplicate",
                 static_cast<unsigned>(id), UiPageIdName(id));
        return false;
    }

    pages_.push_back(std::move(page));
    return true;
}

IUiPage* UiPageRegistry::Get(UiPageId id) const {
    for (const auto& page : pages_) {
        if (page && page->Id() == id) {
            return page.get();
        }
    }
    return nullptr;
}

IUiPage* UiPageRegistry::Active() const {
    return has_active_ ? active_page_ : nullptr;
}

UiPageId UiPageRegistry::ActiveId() const {
    return active_id_;
}

bool UiPageRegistry::HasActive() const {
    return has_active_;
}

bool UiPageRegistry::SwitchTo(UiPageId id) {
    IUiPage* next = Get(id);
    if (!next) {
        ESP_LOGW(TAG, "UI_PAGE switch to=%s result=fail reason=not_registered", UiPageIdName(id));
        return false;
    }

    next->Build();
    if (next->Screen() == nullptr) {
        ESP_LOGW(TAG, "UI_PAGE switch to=%s result=fail reason=screen_null", UiPageIdName(id));
        return false;
    }

    if (has_active_ && active_page_ == next) {
        return true;
    }

    const UiPageId from_id = active_id_;
    if (has_active_ && active_page_) {
        active_page_->OnHide();
    }

    lv_screen_load(next->Screen());
    next->OnShow();
    active_page_ = next;
    active_id_ = id;
    has_active_ = true;

    return true;
}

void UiPageRegistry::Dispatch(const UiPageEvent& event, bool only_active) {
    int consumed = 0;
    if (only_active) {
        if (has_active_ && active_page_ && active_page_->HandleEvent(event)) {
            consumed = 1;
        }
    } else {
        for (const auto& page : pages_) {
            if (page && page->HandleEvent(event)) {
                ++consumed;
            }
        }
    }

    ESP_LOGI(TAG, "UI_PAGE event type=%u consumed=%d only_active=%d",
             static_cast<unsigned>(event.type), consumed, only_active ? 1 : 0);
}

void UiPageRegistry::ForEach(const std::function<void(IUiPage*)>& fn) {
    if (!fn) {
        return;
    }
    for (const auto& page : pages_) {
        if (page) {
            fn(page.get());
        }
    }
}

void UiPageRegistry::Reset() {
    pages_.clear();
    active_page_ = nullptr;
    active_id_ = UiPageId::Vocabulary;
    has_active_ = false;
}
