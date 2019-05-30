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

class PianoRoll;
class MidiTrack;

#include "MidiEventComponent.h"
#include "Note.h"
#include "Clip.h"

class NoteComponent final : public MidiEventComponent, Label::Listener
{
public:

    NoteComponent(PianoRoll &gridRef, const Note &note,
        const Clip &clip, bool ghostMode = false) noexcept;

    enum class State : uint8
    {
        None,
        Initializing,       // changes length & key
        Dragging,           // changes beat & key
        ResizingRight,      // changes length
        ResizingLeft,       // changes beat & length
        GroupScalingRight,  
        GroupScalingLeft,
        Tuning
    };
    
    //===------------------------------------------------------------------===//
    // Helpers
    //===------------------------------------------------------------------===//

    inline int getKey() const noexcept { return this->note.getKey(); }
    inline float getLength() const noexcept { return this->note.getLength(); }
    inline float getVelocity() const noexcept { return this->note.getVelocity(); }
    inline const Note &getNote() const noexcept { return this->note; }
    inline const Clip &getClip() const noexcept { return this->clip; }

    PianoRoll &getRoll() const noexcept;

    void updateColours() override;

    //===------------------------------------------------------------------===//
    // MidiEventComponent
    //===------------------------------------------------------------------===//

    void setSelected(bool selected) override;
    const String &getSelectionGroupId() const noexcept override;
    const String &getId() const noexcept override { return this->note.getId(); }
    float getBeat() const noexcept override { return this->note.getBeat(); }

    //===------------------------------------------------------------------===//
    // Component
    //===------------------------------------------------------------------===//
    void labelTextChanged(Label *labelThatHasChanged) override;
    bool keyStateChanged(bool isKeyDown) override;
    void modifierKeysChanged(const ModifierKeys &modifiers) override;
    void mouseMove(const MouseEvent &e) override;
    void mouseDown(const MouseEvent &e) override;
    void mouseDrag(const MouseEvent &e) override;
    void mouseUp(const MouseEvent &e) override;
    void mouseDoubleClick(const MouseEvent &e) override;
    void paint(Graphics &g) noexcept override;
    void resized() override;

private:
    class EditableLabel : public Label
    {
    public:
        void mouseDoubleClick(const MouseEvent& e)
        {
            if (isEditableOnDoubleClick()
                && isEnabled()
                && !e.mods.isPopupMenu())
                showEditor();
        }
    };
    const Note &note;
    const Clip &clip;
    EditableLabel label;

private:

    struct NoteEditAnchor final
    {
        NoteEditAnchor &operator= (const Note &note)
        {
            this->key = note.getKey();
            this->beat = note.getBeat();
            this->length = note.getLength();
            this->velocity = note.getVelocity();
            return *this;
        }

        inline int getKey() const noexcept { return this->key; }
        inline float getBeat() const noexcept { return this->beat; }
        inline float getLength() const noexcept { return this->length; }
        inline float getVelocity() const noexcept { return this->velocity; }

    private:
        int key = 0;
        float beat = 0.f;
        float length = 0.f;
        float velocity = 0.f;
    };

    struct GroupScaleAnchor final
    {
        GroupScaleAnchor() = default;
        GroupScaleAnchor(float beat, float length) : beat(beat), length(length) {}
        inline float getBeat() const noexcept { return this->beat; }
        inline float getLength() const noexcept { return this->length; }
    private:
        float beat = 0.f;
        float length = 0.f;
    };

    NoteEditAnchor anchor;
    GroupScaleAnchor groupScalingAnchor;

private:

    bool belongsTo(const WeakReference<MidiTrack> &track, const Clip &clip) const noexcept;
    void switchActiveSegmentToSelected(bool zoomToScope) const;

    bool isInitializing() const;
    void startInitializing();
    bool getInitializingDelta(const MouseEvent &e, float &deltaLength, int &deltaKey) const;
    Note continueInitializing(float deltaLength, int deltaKey, bool sendMidi) const noexcept;
    void endInitializing();

    void startResizingRight(bool sendMidiMessage);
    bool getResizingRightDelta(const MouseEvent &e, float &deltaLength) const;
    Note continueResizingRight(float deltaLength) const noexcept;
    void endResizingRight();
    
    void startResizingLeft(bool sendMidiMessage);
    bool getResizingLeftDelta(const MouseEvent &e, float &deltaLength) const;
    Note continueResizingLeft(float deltaLength) const noexcept;
    void endResizingLeft();
    
    void startTuning();
    Note continueTuning(const MouseEvent &e) const noexcept;
    Note continueTuningLinear(float delta) const noexcept;
    Note continueTuningMultiplied(float factor) const noexcept;
    Note continueTuningSine(float factor, float midline, float phase) const noexcept;
    void endTuning();
    
    void startGroupScalingRight(float groupStartBeat);
    bool getGroupScaleRightFactor(const MouseEvent &e, float &absScaleFactor) const;
    Note continueGroupScalingRight(float absScaleFactor) const noexcept;
    void endGroupScalingRight();
    
    void startGroupScalingLeft(float groupEndBeat);
    bool getGroupScaleLeftFactor(const MouseEvent &e, float &absScaleFactor) const;
    Note continueGroupScalingLeft(float absScaleFactor) const noexcept;
    void endGroupScalingLeft();
    
    void startDragging(bool sendMidiMessage);
    bool getDraggingDelta(const MouseEvent &e, float &deltaBeat, int &deltaKey);
    Note continueDragging(float deltaBeat, int deltaKey, bool sendMidiMessage) const noexcept;
    void endDragging(bool sendMidiMessage = true);
    
    bool canResize() const noexcept;

private:

    State state;

    Colour colour;
    Colour colourLighter;
    Colour colourDarker;
    Colour colourVolume;

    friend class PianoRoll;
    friend class NoteResizerLeft;
    friend class NoteResizerRight;
    friend struct SequencerOperations;

    bool firstChangeDone;
    void checkpointIfNeeded();

    bool shouldGoQuickSelectLayerMode(const ModifierKeys &modifiers) const;
    void setQuickSelectLayerMode(bool value);

    void stopSound();
    void sendNoteOn(int noteKey, float velocity) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoteComponent)
    
};
