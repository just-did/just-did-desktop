#pragma once

#include <QString>
#include <QList>
#include <algorithm>

struct NetworkAddressCandidate {
    QString address;
    QString interfaceName;
    bool physical = false;
    bool privateAddress = false;
    bool tunnel = false;
};

inline QString selectLanAddress(QList<NetworkAddressCandidate> candidates)
{
    candidates.erase(std::remove_if(candidates.begin(), candidates.end(),
                                    [](const NetworkAddressCandidate &candidate) {
                                        return candidate.tunnel || candidate.address.isEmpty();
                                    }),
                     candidates.end());
    std::sort(candidates.begin(), candidates.end(),
              [](const NetworkAddressCandidate &a, const NetworkAddressCandidate &b) {
        const int aScore = (a.privateAddress ? 100 : 0) + (a.physical ? 50 : 0);
        const int bScore = (b.privateAddress ? 100 : 0) + (b.physical ? 50 : 0);
        if (aScore != bScore) return aScore > bScore;
        if (a.interfaceName != b.interfaceName) return a.interfaceName < b.interfaceName;
        return a.address < b.address;
    });
    return candidates.isEmpty() ? QStringLiteral("127.0.0.1") : candidates.first().address;
}
