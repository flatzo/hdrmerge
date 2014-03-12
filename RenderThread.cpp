/*
 *  HDRMerge - HDR exposure merging software.
 *  Copyright 2012 Javier Celaya
 *  jcelaya@gmail.com
 *
 *  This file is part of HDRMerge.
 *
 *  HDRMerge is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  HDRMerge is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with HDRMerge. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <cmath>
#include "RenderThread.hpp"
#include <QTime>


RenderThread::RenderThread(ExposureStack * es, float gamma, QObject * parent)
    : QThread(parent), restart(false), abort(false), images(es), minx(0), miny(0), maxx(0), maxy(0), scale(0) {
    setGamma(gamma);
}


RenderThread::~RenderThread() {
    abort = true;
    condition.wakeOne();
    wait();
    delete images;
}


void RenderThread::setGamma(float g) {
    mutex.lock();
    g = 1.0f / g;
    for (int i = 0; i < 65536; i++)
        gamma[i] = (int)std::floor(65536.0f * std::pow(i / 65536.0f, g)) >> 8;
    mutex.unlock();
}


void RenderThread::setExposureThreshold(int i, int th) {
    mutex.lock();
    images->setThreshold(i, ((th + 1) << 8) - 1);
    restart = true;
    mutex.unlock();
    condition.wakeOne();
}


void RenderThread::setExposureRelativeEV(int i, double re) {
    mutex.lock();
    images->setRelativeExposure(i, re);
    restart = true;
    mutex.unlock();
    condition.wakeOne();
}


void RenderThread::setImageViewport(int x, int y, int w, int h, int newScale) {
    mutex.lock();
    if (newScale != scale) {
        restart = true;
        scale = newScale;
    }
    minx = x;
    miny = y;
    maxx = x + w;
    maxy = y + h;
    mutex.unlock();
    if (restart)
        condition.wakeOne();
}


void RenderThread::addPixels(int i, int x, int y, int radius) {
    mutex.lock();
    images->addPixels(i, x, y, radius);
    QImage a(2*radius + 1, 2*radius + 1, QImage::Format_RGB32);
    doRender(x - radius, y - radius, x + radius + 1, y + radius + 1, a, true);
    emit renderedImage(x - radius, y - radius, images->getWidth(), images->getHeight(), a);
    mutex.unlock();
}


void RenderThread::removePixels(int i, int x, int y, int radius) {
    mutex.lock();
    images->removePixels(i, x, y, radius);
    QImage a(2*radius + 1, 2*radius + 1, QImage::Format_RGB32);
    doRender(x - radius, y - radius, x + radius + 1, y + radius + 1, a, true);
    emit renderedImage(x - radius, y - radius, images->getWidth(), images->getHeight(), a);
    mutex.unlock();
}


void RenderThread::doRender(unsigned int minx, unsigned int miny, unsigned int maxx, unsigned int maxy, QImage & image, bool ignoreRestart) {
    QTime t;
    t.start();
    // Iterate through pixels
    for (unsigned int row = miny; (!restart || ignoreRestart) && row < maxy; row++) {
        if (abort) return;

        QRgb * scanLine = reinterpret_cast<QRgb *>(image.scanLine(row - miny));
        for (unsigned int col = minx; col < maxx; col++) {
            double rr, gg, bb;
            images->rgb(col, row, rr, gg, bb);
            int r = (int)rr, g = (int)gg, b = (int)bb;
            // Apply gamma correction
            *scanLine++ = qRgb(gamma[r], gamma[g], gamma[b]);
        }
    }
}


void RenderThread::run() {
    unsigned int _minx = 0, _miny = 0, _maxx = 0, _maxy = 0;
    forever {
        if (abort) return;

        if (_maxy > 0) {
        //if (!restart && _maxy > 0) {
            QImage a(_maxx - _minx, _maxy - _miny, QImage::Format_RGB32);
            doRender(_minx, _miny, _maxx, _maxy, a, true);
            emit renderedImage(_minx, _miny, images->getWidth(), images->getHeight(), a);
            yieldCurrentThread();
        }

        QImage b(images->getWidth(), images->getHeight(), QImage::Format_RGB32);
        doRender(0, 0, images->getWidth(), images->getHeight(), b);
        mutex.lock();
        if (!restart) {
            emit renderedImage(0, 0, images->getWidth(), images->getHeight(), b);
            // Wait until render is called
            condition.wait(&mutex);
        }
        restart = false;
        _minx = minx;
        _miny = miny;
        _maxx = maxx;
        _maxy = maxy;
        images->setScale(scale);
        mutex.unlock();
    }
}

