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

#include "Common.h"
#include "Note.h"
#include "MidiSequence.h"
#include "SerializationKeys.h"

Note::Note() noexcept : MidiEvent(nullptr, Type::Note, 0.f) {}

Note::Note(WeakReference<MidiSequence> owner,
    int keyVal, float beatVal,
    float lengthVal, float velocityVal) noexcept :
    MidiEvent(owner, Type::Note, beatVal),
    key(keyVal),
    length(lengthVal),
    velocity(velocityVal),
	lyric("a") {}

Note::Note(WeakReference<MidiSequence> owner, const Note &parametersToCopy) noexcept :
    MidiEvent(owner, parametersToCopy),
    key(parametersToCopy.key),
    length(parametersToCopy.length),
    velocity(parametersToCopy.velocity),
	lyric(parametersToCopy.lyric),
    tuplet(parametersToCopy.tuplet) {}

void Note::exportMessages(MidiMessageSequence &outSequence,
    double timeOffset, double timeFactor) const noexcept
{
    const auto finalKey = this->key + 0;
    const auto finalVolume = this->velocity * 1;
    const auto tupletLength = this->length / float(this->tuplet);

    for (int i = 0; i < this->tuplet; ++i)
    {
        const float tupletStart = this->beat + tupletLength * float(i);

        // slightly adjust volume for tuplet sequence: factor fading from 1 to 0.9;
        // this should sound anyway better than the same volume for all tuplets,
        // but, in future user should have some kind of control over it
        // (like implement auto curves for individual notes?)
        const float tupletVolume = finalVolume * (1.f - float(i) / 100.f);

        MidiMessage eventNoteOn(MidiMessage::noteOn(this->getTrackChannel(), finalKey, tupletVolume));
        const double startTime = (tupletStart + 0) * timeFactor;
        eventNoteOn.setTimeStamp(startTime);
        outSequence.addEvent(eventNoteOn, timeOffset);

        // here, when having odd tuplet, note-off event time might end up
        // being slightly after next event's start time, due to rounding errors,
        // e.g. 17.333333969116211 -> 18.666667938232422
        //                            18.666666030883789 -> 20.000000000000000;
        // just having some offset for every note-off will mess up midi export
        // for events with accurately aligned timestamps, which sucks,
        // (i.e. having 19.999990000000 instead of 20.000000000000000)
        // but, since even tuplets will always have accurate timestamps,
        // we can subtract some little time offset only for odd tuplets
        // to make sure end/start times of neighbor notes never overlap:
        const double oddTupletFix = double(i % 2) / 1000;

        MidiMessage eventNoteOff(MidiMessage::noteOff(this->getTrackChannel(), finalKey));
        const double endTime = (tupletStart + tupletLength + 0) * timeFactor - oddTupletFix;
        eventNoteOff.setTimeStamp(endTime);
        outSequence.addEvent(eventNoteOff, timeOffset);
    }
}

Note Note::copyWithNewId(WeakReference<MidiSequence> owner) const noexcept
{
    Note n(*this);
    if (owner != nullptr)
    {
        n.sequence = owner;
    }
    
    n.id = n.createId();
    return n;
}

#define MIN_LENGTH (1.f / TICKS_PER_BEAT)

Note Note::withKey(Key newKey) const noexcept
{
    Note other(*this);
    other.key = jlimit(0, 128, newKey);
    return other;
}

Note Note::withBeat(float newBeat) const noexcept
{
    Note other(*this);
    other.beat = roundBeat(newBeat);
    return other;
}

Note Note::withKeyBeat(Key newKey, float newBeat) const noexcept
{
    Note other(*this);
    other.key = jlimit(0, 128, newKey);
    other.beat = roundBeat(newBeat);
    return other;
}

Note Note::withKeyLength(Key newKey, float newLength) const noexcept
{
    Note other(*this);
    other.key = jlimit(0, 128, newKey);
    other.length = jmax(MIN_LENGTH, roundBeat(newLength));
    return other;
}

Note Note::withDeltaBeat(float deltaPosition) const noexcept
{
    Note other(*this);
    other.beat = roundBeat(other.beat + deltaPosition);
    return other;
}

Note Note::withDeltaKey(Key deltaKey) const noexcept
{
    Note other(*this);
    other.key = jlimit(0, 128, other.key + deltaKey);
    return other;
}

Note Note::withLength(float newLength) const noexcept
{
    Note other(*this);
    other.length = jmax(MIN_LENGTH, roundBeat(newLength));
    return other;
}

Note Note::withDeltaLength(float deltaLength) const noexcept
{
    Note other(*this);
    other.length = jmax(MIN_LENGTH, roundBeat(other.length + deltaLength));
    return other;
}

Note Note::withVelocity(float newVelocity) const noexcept
{
    Note other(*this);
    other.velocity = jlimit(0.f, 1.f, newVelocity);
    return other;
}

Note Note::withTuplet(Tuplet tuplet) const noexcept
{
    Note other(*this);
    // what would be the sane upper limit? like 9
    other.tuplet = jlimit(Tuplet(1), Tuplet(9), tuplet);
    return other;
}

Note Note::withParameters(const ValueTree &parameters) const noexcept
{
    Note n(*this);
    n.deserialize(parameters);
    return n;
}

Note Note::withLyric(String lyric) const noexcept
{
    Note other(*this);
    other.lyric = lyric;
    return other;
}

//===----------------------------------------------------------------------===//
// Accessors
//===----------------------------------------------------------------------===//

Note::Key Note::getKey() const noexcept
{
    return this->key;
}

float Note::getLength() const noexcept
{
    return this->length;
}

float Note::getVelocity() const noexcept
{
    return this->velocity;
}
String Note::getLyric() const noexcept
{
    return this->lyric;
}
Note::Tuplet Note::getTuplet() const noexcept
{
    return this->tuplet;
}

//===----------------------------------------------------------------------===//
// Serializable
//===----------------------------------------------------------------------===//

ValueTree Note::serialize() const noexcept
{
    using namespace Serialization;
    ValueTree tree(Midi::note);
    tree.setProperty(Midi::id, this->id, nullptr);
    tree.setProperty(Midi::key, this->key, nullptr);
    tree.setProperty(Midi::timestamp, int(this->beat * TICKS_PER_BEAT), nullptr);
    tree.setProperty(Midi::length, int(this->length * TICKS_PER_BEAT), nullptr);
    tree.setProperty(Midi::volume, int(this->velocity * VELOCITY_SAVE_ACCURACY), nullptr);
    tree.setProperty(Midi::text, this->lyric, nullptr);
    if (this->tuplet > 1)
    {
        tree.setProperty(Midi::tuplet, this->tuplet, nullptr);
    }
    return tree;
}

void Note::deserialize(const ValueTree &tree) noexcept
{
    this->reset();
    using namespace Serialization;
    this->id = tree.getProperty(Midi::id);
    this->key = tree.getProperty(Midi::key);
    this->beat = float(tree.getProperty(Midi::timestamp)) / TICKS_PER_BEAT;
    this->length = float(tree.getProperty(Midi::length)) / TICKS_PER_BEAT;
    const auto vol = float(tree.getProperty(Midi::volume)) / VELOCITY_SAVE_ACCURACY;
    this->velocity = jmax(jmin(vol, 1.f), 0.f);
    this->lyric = tree.getProperty(Midi::text);
    this->tuplet = Tuplet(int(tree.getProperty(Midi::tuplet, 1)));
}

void Note::reset() noexcept {}

void Note::applyChanges(const Note &other) noexcept
{
    jassert(this->id == other.id);
    this->beat = roundBeat(other.beat);
    this->key = other.key;
    this->length = other.length;
    this->velocity = other.velocity;
    this->lyric = other.lyric;
    this->tuplet = other.tuplet;
}

int Note::compareElements(const Note *const first, const Note *const second) noexcept
{
    if (first == second) { return 0; }

    const float beatDiff = first->beat - second->beat;
    const int beatResult = (beatDiff > 0.f) - (beatDiff < 0.f);
    if (beatResult != 0) { return beatResult; }

    //const float lenDiff = (first->beat + first->length) - (second->beat + second->length);
    //const int lenResult = (lenDiff > 0.f) - (lenDiff < 0.f);
    //if (lenResult != 0) { return lenResult; }

    const int keyDiff = first->key - second->key;
    const int keyResult = (keyDiff > 0) - (keyDiff < 0);
    if (keyResult != 0) { return keyResult; }

    return first->getId().compare(second->getId());
}
