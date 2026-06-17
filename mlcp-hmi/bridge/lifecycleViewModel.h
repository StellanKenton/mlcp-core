#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

#include "bridge/hmiBackend.h"
#include "bridge/lifecycleUiState.h"

namespace mlcp::hmi::bridge {

class LifecycleViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(int stateCode READ stateCode NOTIFY lifecycleChanged)
    Q_PROPERTY(QString stateText READ stateText NOTIFY lifecycleChanged)
    Q_PROPERTY(QString severityText READ severityText NOTIFY lifecycleChanged)
    Q_PROPERTY(bool alarmActive READ alarmActive NOTIFY lifecycleChanged)
    Q_PROPERTY(bool degraded READ degraded NOTIFY lifecycleChanged)

public:
    explicit LifecycleViewModel(
        const IHmiBackend& backend,
        QObject* parent = nullptr);

    int stateCode() const;
    QString stateText() const;
    QString severityText() const;
    bool alarmActive() const;
    bool degraded() const;

signals:
    void lifecycleChanged();
    void stateTextChanged();

private:
    void refreshLifecycle();
    void applyState(const LifecycleUiState& state);

    const IHmiBackend& backend_;
    QTimer refreshTimer_;
    LifecycleUiState state_;
};

} // namespace mlcp::hmi::bridge
