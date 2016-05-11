#include <QDomNode>
#include <QPaintEvent>
#include <QPainter>

#include "waveform/renderers/waveformrenderbeat.h"

#include "control/controlobject.h"
#include "track/beats.h"
#include "track/track.h"
#include "waveform/renderers/waveformwidgetrenderer.h"
#include "widget/wskincolor.h"
#include "widget/wwidget.h"

WaveformRenderBeat::WaveformRenderBeat(WaveformWidgetRenderer* waveformWidgetRenderer)
        : WaveformRendererAbstract(waveformWidgetRenderer) {
    m_beats.resize(128);
    m_beatsFirstInBar.resize(128);
}

WaveformRenderBeat::~WaveformRenderBeat() {
}

void WaveformRenderBeat::setup(const QDomNode& node, const SkinContext& context) {
    m_beatColor.setNamedColor(context.selectString(node, "BeatColor"));
    m_beatColor = WSkinColor::getCorrectColor(m_beatColor).toRgb();

    // TODO (m0r13) get color from skin
    m_firstBeatInBarColor = Qt::red;

    if (m_beatColor.alphaF() > 0.99)
        m_beatColor.setAlphaF(0.9);
}

void WaveformRenderBeat::draw(QPainter* painter, QPaintEvent* /*event*/) {
    TrackPointer trackInfo = m_waveformRenderer->getTrackInfo();

    if (!trackInfo)
        return;

    BeatsPointer trackBeats = trackInfo->getBeats();
    if (!trackBeats)
        return;
    double firstBeatOfTrack = trackBeats->findNextBeat(0);

    const int trackSamples = m_waveformRenderer->getTrackSamples();
    if (trackSamples <= 0) {
        return;
    }

    const double firstDisplayedPosition = m_waveformRenderer->getFirstDisplayedPosition();
    const double lastDisplayedPosition = m_waveformRenderer->getLastDisplayedPosition();

    // qDebug() << "trackSamples" << trackSamples
    //          << "firstDisplayedPosition" << firstDisplayedPosition
    //          << "lastDisplayedPosition" << lastDisplayedPosition;

    std::unique_ptr<BeatIterator> it(trackBeats->findBeats(
            firstDisplayedPosition * trackSamples, lastDisplayedPosition * trackSamples));

    // if no beat do not waste time saving/restoring painter
    if (!it || !it->hasNext()) {
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    QPen beatPen(m_beatColor);
    beatPen.setWidthF(1);
    painter->setPen(beatPen);
    QPen firstBeatInBarPen(m_firstBeatInBarColor);
    firstBeatInBarPen.setWidth(2.5);

    const float rendererHeight = m_waveformRenderer->getHeight();

    // TODO (m0r13) maybe fancify that thing with two types of lines
    int beatCount = 0, beatFirstInBarCount = 0;

    while (it->hasNext()) {
        int beatPosition = it->next();
        double xBeatPoint = m_waveformRenderer->transformSampleIndexInRendererWorld(beatPosition);

        xBeatPoint = qRound(xBeatPoint);

        // If we don't have enough space, double the size.
        if (beatCount >= m_beats.size()) {
            m_beats.resize(m_beats.size() * 2);
        }
        if (beatFirstInBarCount >= m_beatsFirstInBar.size()) {
            m_beatsFirstInBar.resize(m_beatsFirstInBar.size() * 2);
        }

        if (it->isFirstInBar()) {
            m_beatsFirstInBar[beatFirstInBarCount++].setLine(xBeatPoint, 0.0f, xBeatPoint, rendererHeight);
        }
        
        m_beats[beatCount++].setLine(xBeatPoint, 0.0f, xBeatPoint, rendererHeight);
    }

    // Make sure to use constData to prevent detaches!
    painter->drawLines(m_beats.constData(), beatCount);
    painter->setPen(firstBeatInBarPen);
    painter->drawLines(m_beatsFirstInBar.constData(), beatFirstInBarCount);

    painter->restore();
}
