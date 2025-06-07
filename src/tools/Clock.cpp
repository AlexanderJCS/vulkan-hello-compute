#include "Clock.h"

#include <sstream>

void raymarcher::tools::TimeEntries::addEntry(double timing) {
    if (recordings == 0) {
        averageTime = timing;
        recordings++;
        return;
    }

    averageTime = (averageTime * recordings + timing) / (recordings + 1);
    recordings++;
}

raymarcher::tools::Clock::Clock() : creationTime(getTime()) {}

double raymarcher::tools::Clock::getTime() {
    return glfwGetTime();
}

double raymarcher::tools::Clock::getAge() const {
    return getTime() - creationTime;
}

void raymarcher::tools::Clock::markFrame() {
    // don't simply compare with 0 because floating point errors.
    // is it likely there's a floating point error here? no. do I want to risk it? also no.
    if (lastFrameTime < 0.000001) {
        lastFrameTime = getTime();
        return;
    }

    double time = getTime();
    frameTime.addEntry(time - lastFrameTime);
    secondToLastFrameTime = lastFrameTime;
    lastFrameTime = time;
}

void raymarcher::tools::Clock::markCategory(const std::string& category) {
    if (!categoryTimes.contains(category)) {
        categoryTimes.emplace(category, TimeEntries{});
    }

    if (lastCategoryRecording < 0.000001) {
        lastCategoryRecording = getTime();
        lastCategory = category;
        return;
    }

    double time = getTime();
    categoryTimes.at(lastCategory).addEntry(time - lastCategoryRecording);
    lastCategory = category;
    lastCategoryRecording = time;
}

std::string raymarcher::tools::Clock::summary() {
    std::ostringstream oss;
    oss << "Timer age: " << getAge() << "s\n";
    oss << "Average frame time: " << frameTime.averageTime * 1000 << "ms\n";

    for (auto & time : categoryTimes) {
        oss << "Average category time | " << time.first << ": " << time.second.averageTime * 1000 << "ms\n";
    }

    return oss.str();
}

unsigned int raymarcher::tools::Clock::getFrameCount() const {
    return frameTime.recordings;
}

double raymarcher::tools::Clock::getAverageFrameTime() const {
    return frameTime.averageTime;
}

double raymarcher::tools::Clock::getAverageCategoryTime(const std::string& category) const {
    return categoryTimes.at(category).averageTime;
}

double raymarcher::tools::Clock::getTimeDelta() const {
    return lastFrameTime - secondToLastFrameTime;
}
