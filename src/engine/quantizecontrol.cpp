// QuantizeControl.cpp
// Created on Sat 5, 2011
// Author: pwhelan

#include <QtDebug>

#include "controlobject.h"
#include "configobject.h"
#include "controlpushbutton.h"
#include "cachingreader.h"
#include "engine/quantizecontrol.h"
#include "engine/enginecontrol.h"
#include "util/assert.h"

QuantizeControl::QuantizeControl(QString group,
                                 ConfigObject<ConfigValue>* pConfig)
        : EngineControl(group, pConfig) {
    // Turn quantize OFF by default. See Bug #898213
    m_pCOQuantizeEnabled = new ControlPushButton(ConfigKey(group, "quantize"));
    m_pCOQuantizeEnabled->setButtonMode(ControlPushButton::TOGGLE);
    m_pCONextBeat = new ControlObject(ConfigKey(group, "beat_next"));
    m_pCONextBeat->set(-1);
    m_pCOPrevBeat = new ControlObject(ConfigKey(group, "beat_prev"));
    m_pCOPrevBeat->set(-1);
    m_pCOClosestBeat = new ControlObject(ConfigKey(group, "beat_closest"));
    m_pCOClosestBeat->set(-1);
}

QuantizeControl::~QuantizeControl() {
    delete m_pCOQuantizeEnabled;
    delete m_pCONextBeat;
    delete m_pCOPrevBeat;
    delete m_pCOClosestBeat;
}

void QuantizeControl::trackLoaded(TrackPointer pTrack) {
    if (m_pTrack) {
        trackUnloaded(m_pTrack);
    }

    if (pTrack) {
        m_pTrack = pTrack;
        m_pBeats = m_pTrack->getBeats();
        connect(m_pTrack.data(), SIGNAL(beatsUpdated()),
                this, SLOT(slotBeatsUpdated()));
        // Initialize prev and next beat as if current position was zero.
        // If there is a cue point, the value will be updated.
        postProcess(0, 0);
    }
}

void QuantizeControl::trackUnloaded(TrackPointer pTrack) {
    Q_UNUSED(pTrack);
    if (m_pTrack) {
        disconnect(m_pTrack.data(), SIGNAL(beatsUpdated()),
                   this, SLOT(slotBeatsUpdated()));
    }
    m_pTrack.clear();
    m_pBeats.clear();
    m_pCOPrevBeat->set(-1);
    m_pCONextBeat->set(-1);
    m_pCOClosestBeat->set(-1);
}

void QuantizeControl::slotBeatsUpdated() {
    if (m_pTrack) {
        m_pBeats = m_pTrack->getBeats();
        postProcess(0, 0);
    }
}

double QuantizeControl::postProcess(const double currentSample,
                                    const int iBufferSize) {
    Q_UNUSED(iBufferSize);

    if (!m_pBeats) {
        return kNoTrigger;
    }

    int iCurrentSample = currentSample;
    DEBUG_ASSERT_AND_HANDLE(even(iCurrentSample)) {
        iCurrentSample--;
    }

    double prevBeat = m_pCOPrevBeat->get();
    double nextBeat = m_pCONextBeat->get();
    double closestBeat = m_pCOClosestBeat->get();

    // Calculate this by hand since we may also want the beat locations themselves
    // and duplicating the work would double the number of mutex locks.
    QPair<double, double> beat_pair = m_pBeats->findPrevNextBeats(iCurrentSample);
    double currentClosestBeat =
            (beat_pair.second - iCurrentSample > iCurrentSample - beat_pair.first) ?
                    beat_pair.first : beat_pair.second;

    if (closestBeat != currentClosestBeat) {
        // findXBeats claims to guarantee evenness.
        DEBUG_ASSERT_AND_HANDLE(even(static_cast<int>(currentClosestBeat))) {
            currentClosestBeat--;
        }
        m_pCOClosestBeat->set(currentClosestBeat);
    }

    if (prevBeat == -1 || nextBeat == -1 ||
        currentSample >= nextBeat || currentSample <= prevBeat) {
        m_pCOPrevBeat->set(beat_pair.first);
        m_pCONextBeat->set(beat_pair.second);
    }

    return kNoTrigger;
}
