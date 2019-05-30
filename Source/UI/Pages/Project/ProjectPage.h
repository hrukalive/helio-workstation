/*
    This file is part of Helio Workstation.

    Helio is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Helio is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Helio. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

//[Headers]
class ProjectNode;

#include "TransportListener.h"
//[/Headers]

#include "../../Themes/PanelBackgroundB.h"
#include "../../Common/MenuButton.h"

class ProjectPage  : public Component,
                     protected TransportListener,
                     protected ChangeListener,
                     public Label::Listener,
                     public Button::Listener
{
public:

    ProjectPage(ProjectNode &parentProject);
    ~ProjectPage();

    //[UserMethods]
    void updateContent();
    //[/UserMethods]

    void paint (Graphics& g) override;
    void resized() override;
    void labelTextChanged (Label* labelThatHasChanged) override;
    void buttonClicked (Button* buttonThatWasClicked) override;
    void visibilityChanged() override;
    void handleCommandMessage (int commandId) override;


private:

    //[UserVariables]

    ProjectNode &project;
    MidiKeyboardState state;
    Atomic<double> totalTimeMs;

    void changeListenerCallback(ChangeBroadcaster *source) override;

    //===----------------------------------------------------------------------===//
    // TransportListener
    //===----------------------------------------------------------------------===//

    void onSeek(double absolutePosition, double currentTimeMs, double totalTimeMs) noexcept override;
    void onTempoChanged(double msPerQuarter) noexcept override {}
    void onTotalTimeChanged(double timeMs) noexcept override;
    void onPlay() noexcept override {}
    void onStop() noexcept override {}

    //[/UserVariables]

    std::unique_ptr<PanelBackgroundB> background;
    std::unique_ptr<Label> projectTitleEditor;
    std::unique_ptr<Label> projectTitleLabel;
    std::unique_ptr<Label> authorEditor;
    std::unique_ptr<Label> authorLabel;
    std::unique_ptr<Label> descriptionEditor;
    std::unique_ptr<Label> descriptionLabel;
    std::unique_ptr<Label> locationLabel;
    std::unique_ptr<Label> locationText;
    std::unique_ptr<Label> contentStatsLabel;
    std::unique_ptr<Label> contentStatsText;
    std::unique_ptr<Label> vcsStatsLabel;
    std::unique_ptr<Label> vcsStatsText;
    std::unique_ptr<Label> startTimeLabel;
    std::unique_ptr<Label> startTimeText;
    std::unique_ptr<Label> lengthLabel;
    std::unique_ptr<Label> lengthText;
    std::unique_ptr<Component> level1;
    std::unique_ptr<Component> level2;
    std::unique_ptr<Label> licenseLabel;
    std::unique_ptr<Label> licenseEditor;
    std::unique_ptr<MenuButton> menuButton;
    std::unique_ptr<ImageButton> revealLocationButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProjectPage)
};


