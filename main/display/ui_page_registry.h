#ifndef UI_PAGE_REGISTRY_H
#define UI_PAGE_REGISTRY_H

#include "ui_page.h"

#include <functional>
#include <memory>
#include <vector>

class UiPageRegistry {
public:
    bool Register(std::unique_ptr<IUiPage> page);
    IUiPage* Get(UiPageId id) const;
    IUiPage* Active() const;
    UiPageId ActiveId() const;
    bool HasActive() const;
    bool SwitchTo(UiPageId id);
    void Dispatch(const UiPageEvent& event, bool only_active = true);
    void ForEach(const std::function<void(IUiPage*)>& fn);
    void Reset();

private:
    std::vector<std::unique_ptr<IUiPage>> pages_;
    IUiPage* active_page_ = nullptr;
    UiPageId active_id_ = UiPageId::Vocabulary;
    bool has_active_ = false;
};

#endif  // UI_PAGE_REGISTRY_H
