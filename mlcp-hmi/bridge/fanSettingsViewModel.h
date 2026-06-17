#pragma once

#include <QObject>
#include <QString>

#include "bridge/fanSettingsUiState.h"
#include "bridge/hmiBackend.h"

namespace mlcp::hmi::bridge {

class FanSettingsViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY availabilityChanged)
    Q_PROPERTY(bool temperatureAutoControl READ temperatureAutoControl NOTIFY configChanged)
    Q_PROPERTY(double targetCelsius READ targetCelsius NOTIFY configChanged)
    Q_PROPERTY(double toleranceCelsius READ toleranceCelsius NOTIFY configChanged)
    Q_PROPERTY(int manualFanDuty READ manualFanDuty NOTIFY configChanged)
    Q_PROPERTY(int maxFanDuty READ maxFanDuty NOTIFY configChanged)
    Q_PROPERTY(int latestFanDuty READ latestFanDuty NOTIFY runtimeSnapshotChanged)
    Q_PROPERTY(bool hasLatestTemperature READ hasLatestTemperature NOTIFY runtimeSnapshotChanged)
    Q_PROPERTY(double latestTemperatureCelsius READ latestTemperatureCelsius
               NOTIFY runtimeSnapshotChanged)
    Q_PROPERTY(QString latestTemperatureText READ latestTemperatureText
               NOTIFY runtimeSnapshotChanged)
    Q_PROPERTY(QString dataStatusText READ dataStatusText NOTIFY dataStatusChanged)
    Q_PROPERTY(QString operationStatusText READ operationStatusText
               NOTIFY operationStatusChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY settingsChanged)
    Q_PROPERTY(int errorCode READ errorCode NOTIFY operationStatusChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY operationStatusChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY operationStatusChanged)

public:
    explicit FanSettingsViewModel(
        IHmiBackend& backend,
        QObject* parent = nullptr);

    bool available() const;
    bool temperatureAutoControl() const;
    double targetCelsius() const;
    double toleranceCelsius() const;
    int manualFanDuty() const;
    int maxFanDuty() const;
    int latestFanDuty() const;
    bool hasLatestTemperature() const;
    double latestTemperatureCelsius() const;
    QString latestTemperatureText() const;
    QString dataStatusText() const;
    QString operationStatusText() const;
    QString statusText() const;
    int errorCode() const;
    QString errorText() const;
    bool hasError() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool applySettings(bool temperatureAutoControl,
                                   double targetCelsius,
                                   int manualFanDuty,
                                   int maxFanDuty);

signals:
    void availabilityChanged();
    void configChanged();
    void runtimeSnapshotChanged();
    void dataStatusChanged();
    void operationStatusChanged();
    void settingsChanged();

private:
    void applyState(const FanSettingsUiState& state);
    void applyRefreshFailure(const UiError& error);
    void setOperationResult(const QString& message, const UiError& error);
    bool applyFanSettingsCommand(const FanSettingsCommand& command);
    mlcp::hmi::service::temp::TempServiceConfig configForCommand(
        const FanSettingsCommand& command);

    IHmiBackend& backend_;
    FanSettingsUiState state_;
};

} // namespace mlcp::hmi::bridge
