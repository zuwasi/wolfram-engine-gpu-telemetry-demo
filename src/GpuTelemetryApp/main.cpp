#include "GpuTelemetryProvider.h"
#include "JsonTelemetry.h"
#include "MathematicaBridge.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QTimer>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>

#include <windows.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using AnalyzeFn = int (*)(const char*, char*, int);

std::vector<GpuTelemetrySample> BuiltInSamples()
{
    GpuTelemetryProvider provider;
    std::vector<GpuTelemetrySample> samples;
    const auto base = std::chrono::system_clock::time_point{std::chrono::milliseconds{1710000000000ll}};
    for (int i = 0; i < 12; ++i) {
        auto sample = provider.ReadSample(0, i);
        sample.timestamp = base + std::chrono::seconds(i);
        samples.push_back(sample);
    }
    return samples;
}

QString DllPath()
{
    return QCoreApplication::applicationDirPath() + QDir::separator() + "GpuAnalysisDll.dll";
}

bool AnalyzeWithDll(const std::vector<GpuTelemetrySample>& samples, QString& output, QString& error)
{
    const QString path = DllPath();
    HMODULE module = LoadLibraryW(reinterpret_cast<LPCWSTR>(path.utf16()));
    if (!module) {
        error = "Could not load " + path;
        return false;
    }
    auto fn = reinterpret_cast<AnalyzeFn>(GetProcAddress(module, "AnalyzeGpuTelemetryJson"));
    if (!fn) {
        FreeLibrary(module);
        error = "AnalyzeGpuTelemetryJson export not found";
        return false;
    }
    const std::string input = SamplesToJson(samples);
    std::array<char, 4096> buffer{};
    const int rc = fn(input.c_str(), buffer.data(), static_cast<int>(buffer.size()));
    FreeLibrary(module);
    if (rc != 0) {
        error = QString("GpuAnalysisDll returned code %1").arg(rc);
        return false;
    }
    output = QString::fromUtf8(buffer.data());
    return true;
}

int RunSelfTest()
{
    QString output;
    QString error;
    if (!AnalyzeWithDll(BuiltInSamples(), output, error)) {
        std::cerr << "SELF-TEST FAILED: " << error.toStdString() << "\n";
        return 1;
    }
    const auto doc = QJsonDocument::fromJson(output.toUtf8());
    if (!doc.isObject() || !doc.object().contains("healthScore")) {
        std::cerr << "SELF-TEST FAILED: invalid analysis JSON\n";
        return 1;
    }
    std::cout << "SELF-TEST OK: " << output.toStdString() << "\n";
    return 0;
}

int RunWolframEngineSelfTest()
{
    const auto result = MathematicaAnalysisEngine{}.Analyze(BuiltInSamples());
    const auto doc = QJsonDocument::fromJson(QString::fromStdString(result.rawJson).toUtf8());
    if (!doc.isObject() || !doc.object().contains("healthScore")) {
        std::cerr << "WOLFRAM ENGINE SELF-TEST FAILED: invalid analysis JSON\n"
                  << result.rawJson << "\n";
        return 1;
    }

    const auto diagnosis = doc.object().value("diagnosis").toString();
    if (diagnosis.contains("failed", Qt::CaseInsensitive)) {
        std::cerr << "WOLFRAM ENGINE SELF-TEST FAILED: " << result.rawJson << "\n";
        return 1;
    }

    std::cout << "WOLFRAM ENGINE SELF-TEST OK: " << result.rawJson << "\n";
    return 0;
}

QString HealthColor(double score)
{
    if (score >= 90.0) return "#16a34a";
    if (score >= 75.0) return "#ca8a04";
    return "#dc2626";
}

QString RiskColor(double risk)
{
    if (risk < 0.25) return "#16a34a";
    if (risk < 0.55) return "#ca8a04";
    return "#dc2626";
}

QString RiskLabel(double risk)
{
    if (risk < 0.25) return "Low";
    if (risk < 0.55) return "Medium";
    return "High";
}

QString Card(const QString& title, const QString& value, const QString& subtitle, const QString& color)
{
    return QString(R"(
        <td style="width:25%; padding:8px; vertical-align:top;">
          <div style="border:1px solid #d8dee9; border-radius:12px; padding:14px; background:#ffffff;">
            <div style="font-size:12px; color:#64748b; text-transform:uppercase; letter-spacing:0.08em;">%1</div>
            <div style="font-size:28px; font-weight:800; color:%4; margin-top:6px;">%2</div>
            <div style="font-size:12px; color:#475569; margin-top:4px;">%3</div>
          </div>
        </td>)")
        .arg(title.toHtmlEscaped(), value.toHtmlEscaped(), subtitle.toHtmlEscaped(), color);
}

QString Bar(const QString& label, double value, const QString& color)
{
    const int percent = static_cast<int>(std::clamp(value * 100.0, 0.0, 100.0));
    return QString(R"(
      <div style="margin:10px 0;">
        <div style="display:flex; justify-content:space-between; font-size:13px; color:#334155;">
          <span>%1</span><span><b>%2%</b></span>
        </div>
        <div style="height:10px; background:#e2e8f0; border-radius:999px; overflow:hidden; margin-top:4px;">
          <div style="height:10px; width:%2%; background:%3;"></div>
        </div>
      </div>)")
        .arg(label.toHtmlEscaped())
        .arg(percent)
        .arg(color);
}

QString StatusBadge(const QString& text, const QString& color)
{
    return QString(R"(<span style="display:inline-block; min-width:72px; text-align:center; padding:4px 10px; border-radius:999px; color:white; background:%1; font-weight:700; font-size:12px;">%2</span>)")
        .arg(color, text.toHtmlEscaped());
}

QString ReportRow(const QString& metric, const QString& value, const QString& status, const QString& color, const QString& interpretation)
{
    return QString(R"(
      <tr>
        <td style="padding:10px 12px; border-bottom:1px solid #e2e8f0; font-weight:700; color:#0f172a;">%1</td>
        <td style="padding:10px 12px; border-bottom:1px solid #e2e8f0; font-family:Consolas, monospace; color:#334155;">%2</td>
        <td style="padding:10px 12px; border-bottom:1px solid #e2e8f0;">%3</td>
        <td style="padding:10px 12px; border-bottom:1px solid #e2e8f0; color:#475569;">%4</td>
      </tr>)")
        .arg(metric.toHtmlEscaped(), value.toHtmlEscaped(), StatusBadge(status, color), interpretation.toHtmlEscaped());
}

QString ReportTable(double health, double predictedTemperature, int anomalyCount, double thermalRisk,
                    double powerInstability, double memoryPressure, double utilizationStability)
{
    const QString healthStatus = health >= 90.0 ? "Excellent" : (health >= 75.0 ? "Stable" : "Attention");
    const QString tempStatus = predictedTemperature < 78.0 ? "Safe" : (predictedTemperature < 88.0 ? "Watch" : "Hot");
    const QString anomalyStatus = anomalyCount == 0 ? "Clear" : (anomalyCount <= 2 ? "Watch" : "Alert");
    const QString utilStatus = utilizationStability >= 0.80 ? "Stable" : (utilizationStability >= 0.55 ? "Variable" : "Unstable");

    QString rows;
    rows += ReportRow("Composite health", QString::number(health, 'f', 1) + " / 100", healthStatus, HealthColor(health),
                      "Overall weighted GPU health score from the Wolfram Engine calculation.");
    rows += ReportRow("Predicted temperature", QString::number(predictedTemperature, 'f', 1) + " °C", tempStatus,
                      tempStatus == "Safe" ? "#16a34a" : (tempStatus == "Watch" ? "#ca8a04" : "#dc2626"),
                      "Projected temperature 60 seconds ahead from the smoothed thermal trend.");
    rows += ReportRow("Thermal risk", QString::number(thermalRisk * 100.0, 'f', 0) + "%", RiskLabel(thermalRisk), RiskColor(thermalRisk),
                      "Risk from current temperature, trend slope, and predicted temperature.");
    rows += ReportRow("Power instability", QString::number(powerInstability * 100.0, 'f', 0) + "%", RiskLabel(powerInstability), RiskColor(powerInstability),
                      "Power variation, rapid power changes, and power-limit pressure.");
    rows += ReportRow("Memory pressure", QString::number(memoryPressure * 100.0, 'f', 0) + "%", RiskLabel(memoryPressure), RiskColor(memoryPressure),
                      "Memory usage level and growth rate during the sample window.");
    rows += ReportRow("Utilization stability", QString::number(utilizationStability * 100.0, 'f', 0) + "%", utilStatus,
                      utilStatus == "Stable" ? "#16a34a" : (utilStatus == "Variable" ? "#ca8a04" : "#dc2626"),
                      "Stability from utilization variance and correlation with power/temperature.");
    rows += ReportRow("Anomaly events", QString::number(anomalyCount), anomalyStatus,
                      anomalyStatus == "Clear" ? "#16a34a" : (anomalyStatus == "Watch" ? "#ca8a04" : "#dc2626"),
                      "Robust z-score outliers detected across temperature, power, or utilization.");

    return QString(R"(
      <div style="margin:12px 8px; border:1px solid #d8dee9; border-radius:14px; background:#ffffff; overflow:hidden;">
        <div style="padding:14px 16px; background:#f1f5f9; border-bottom:1px solid #d8dee9;">
          <div style="font-size:14px; font-weight:800; color:#0f172a;">Color-coded report table</div>
          <div style="font-size:12px; color:#64748b; margin-top:2px;">Green = good, amber = watch, red = attention required</div>
        </div>
        <table width="100%" cellspacing="0" cellpadding="0" style="border-collapse:collapse; background:#ffffff;">
          <tr style="background:#f8fafc; color:#64748b; font-size:12px; text-transform:uppercase; letter-spacing:0.06em;">
            <th align="left" style="padding:9px 12px; border-bottom:1px solid #e2e8f0;">Metric</th>
            <th align="left" style="padding:9px 12px; border-bottom:1px solid #e2e8f0;">Value</th>
            <th align="left" style="padding:9px 12px; border-bottom:1px solid #e2e8f0;">Status</th>
            <th align="left" style="padding:9px 12px; border-bottom:1px solid #e2e8f0;">Meaning</th>
          </tr>
          %1
        </table>
      </div>)")
        .arg(rows);
}

QString FormatAnalysisHtml(const QJsonDocument& doc, const QString& engineName, const QString& rawJson)
{
    if (!doc.isObject()) return "<pre>" + rawJson.toHtmlEscaped() + "</pre>";

    const QJsonObject o = doc.object();
    const double health = o.value("healthScore").toDouble();
    const double thermalRisk = o.value("thermalRisk").toDouble();
    const double powerInstability = o.value("powerInstability").toDouble();
    const double memoryPressure = o.value("memoryPressure").toDouble();
    const double utilizationStability = o.value("utilizationStability").toDouble();
    const double predictedTemperature = o.value("predictedTemperature60s").toDouble();
    const int anomalyCount = o.value("anomalyCount").toInt();
    const QString diagnosis = o.value("diagnosis").toString();
    const QString healthColor = HealthColor(health);

    return QString(R"(
      <html>
      <body style="font-family:'Segoe UI', Arial, sans-serif; background:#f8fafc; color:#0f172a;">
        <div style="padding:6px 4px 14px 4px;">
          <div style="font-size:12px; color:#64748b; text-transform:uppercase; letter-spacing:0.10em;">Analysis engine</div>
          <div style="font-size:18px; font-weight:700; margin-top:2px;">%1</div>
        </div>

        <table width="100%" cellspacing="0" cellpadding="0"><tr>
          %2
          %3
          %4
          %5
        </tr></table>

        <div style="margin:12px 8px; border:1px solid #d8dee9; border-radius:14px; background:#ffffff; padding:16px;">
          <div style="font-size:12px; color:#64748b; text-transform:uppercase; letter-spacing:0.08em;">Diagnosis</div>
          <div style="font-size:20px; font-weight:700; color:%11; margin-top:6px;">%6</div>
        </div>

        %16

        <table width="100%" cellspacing="0" cellpadding="0"><tr>
          <td style="width:50%; padding:8px; vertical-align:top;">
            <div style="border:1px solid #d8dee9; border-radius:14px; background:#ffffff; padding:16px;">
              <div style="font-size:14px; font-weight:700; margin-bottom:8px;">Risk breakdown</div>
              %7
              %8
              %9
            </div>
          </td>
          <td style="width:50%; padding:8px; vertical-align:top;">
            <div style="border:1px solid #d8dee9; border-radius:14px; background:#ffffff; padding:16px;">
              <div style="font-size:14px; font-weight:700; margin-bottom:8px;">Operational interpretation</div>
              <ul style="margin-top:8px; color:#334155; line-height:1.5;">
                <li>Thermal risk is <b>%12</b>.</li>
                <li>Power behavior is <b>%13</b>.</li>
                <li>Memory pressure is <b>%14</b>.</li>
                <li>Utilization stability is <b>%15%</b>.</li>
              </ul>
            </div>
          </td>
        </tr></table>

        <div style="margin:12px 8px; color:#64748b; font-size:11px;">
          Raw JSON is still available for debugging:<br/>
          <pre style="white-space:pre-wrap; background:#0f172a; color:#e2e8f0; border-radius:10px; padding:10px;">%10</pre>
        </div>
      </body>
      </html>)")
      .arg(engineName.toHtmlEscaped())
      .arg(Card("Health score", QString::number(health, 'f', 1), "Composite 0-100 score", healthColor))
      .arg(Card("Predicted temp", QString::number(predictedTemperature, 'f', 1) + " °C", "60 second forecast", RiskColor((predictedTemperature - 70.0) / 25.0)))
      .arg(Card("Anomalies", QString::number(anomalyCount), "Robust z-score events", anomalyCount == 0 ? "#16a34a" : "#dc2626"))
      .arg(Card("Util stability", QString::number(utilizationStability * 100.0, 'f', 0) + "%", "Correlation/variance model", "#2563eb"))
      .arg(diagnosis.toHtmlEscaped())
      .arg(Bar("Thermal risk", thermalRisk, RiskColor(thermalRisk)))
      .arg(Bar("Power instability", powerInstability, RiskColor(powerInstability)))
      .arg(Bar("Memory pressure", memoryPressure, RiskColor(memoryPressure)))
      .arg(rawJson.toHtmlEscaped())
      .arg(healthColor)
      .arg(RiskLabel(thermalRisk).toHtmlEscaped())
      .arg(RiskLabel(powerInstability).toHtmlEscaped())
      .arg(RiskLabel(memoryPressure).toHtmlEscaped())
      .arg(static_cast<int>(std::clamp(utilizationStability * 100.0, 0.0, 100.0)))
      .arg(ReportTable(health, predictedTemperature, anomalyCount, thermalRisk, powerInstability, memoryPressure, utilizationStability));
}

class DashboardWindow final : public QMainWindow
{
public:
    DashboardWindow()
    {
        setWindowTitle("GPU Telemetry Dashboard - Wolfram Engine to Native C++ Demo");
        resize(1100, 720);
        setMinimumSize(900, 560);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        statusBar()->showMessage("Drag window edges to resize. Drag splitters between panels to resize table/report/log areas.");
        auto* central = new QWidget(this);
        auto* layout = new QVBoxLayout(central);
        layout->setContentsMargins(10, 10, 10, 10);
        layout->setSpacing(8);

        auto* controls = new QGroupBox("Sampling Controls", central);
        controls->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        auto* grid = new QGridLayout(controls);
        gpuCombo_ = new QComboBox(controls);
        for (const auto& gpu : provider_.EnumerateGpus()) gpuCombo_->addItem(QString::fromStdString(gpu));
        durationSpin_ = new QSpinBox(controls);
        durationSpin_->setRange(5, 60);
        durationSpin_->setSingleStep(5);
        durationSpin_->setValue(30);
        engineCombo_ = new QComboBox(controls);
        engineCombo_->addItems({"Wolfram Engine Calculation Engine", "Native C++ DLL", "Mock Engine"});
        startButton_ = new QPushButton("Start", controls);
        stopButton_ = new QPushButton("Stop", controls);
        stopButton_->setEnabled(false);
        progress_ = new QProgressBar(controls);
        grid->addWidget(new QLabel("GPU"), 0, 0);
        grid->addWidget(gpuCombo_, 0, 1);
        grid->addWidget(new QLabel("Duration (seconds)"), 0, 2);
        grid->addWidget(durationSpin_, 0, 3);
        grid->addWidget(new QLabel("Engine"), 0, 4);
        grid->addWidget(engineCombo_, 0, 5);
        grid->addWidget(startButton_, 0, 6);
        grid->addWidget(stopButton_, 0, 7);
        grid->addWidget(progress_, 1, 0, 1, 8);
        layout->addWidget(controls);

        auto* splitter = new QSplitter(Qt::Vertical, central);
        splitter->setChildrenCollapsible(false);
        splitter->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        table_ = new QTableWidget(0, 7, splitter);
        table_->setHorizontalHeaderLabels({"#", "Temp C", "Fan %", "Power W", "GPU %", "Mem %", "Throttle"});
        table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        splitter->addWidget(table_);

        analysis_ = new QTextEdit(splitter);
        analysis_->setReadOnly(true);
        analysis_->setPlaceholderText("Analysis results will appear here after sampling.");
        analysis_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        splitter->addWidget(analysis_);

        log_ = new QPlainTextEdit(splitter);
        log_->setReadOnly(true);
        log_->setMaximumBlockCount(500);
        log_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        splitter->addWidget(log_);
        splitter->setStretchFactor(0, 2);
        splitter->setStretchFactor(1, 3);
        splitter->setStretchFactor(2, 1);
        splitter->setSizes({260, 360, 140});
        layout->addWidget(splitter, 1);
        setCentralWidget(central);

        timer_.setInterval(1000);
        connect(startButton_, &QPushButton::clicked, this, [this] { StartSampling(); });
        connect(stopButton_, &QPushButton::clicked, this, [this] { StopSampling(false); });
        connect(&timer_, &QTimer::timeout, this, [this] { CollectSample(); });
        log_->appendPlainText("Ready. First usage is Wolfram Engine as the calculation engine via wolframscript. Native C++ DLL is the later deployment engine.");
    }

private:
    void StartSampling()
    {
        samples_.clear();
        table_->setRowCount(0);
        analysis_->clear();
        sampleIndex_ = 0;
        targetSamples_ = durationSpin_->value();
        progress_->setValue(0);
        startButton_->setEnabled(false);
        stopButton_->setEnabled(true);
        log_->appendPlainText(QString("Sampling %1 seconds...").arg(targetSamples_));
        timer_.start();
        CollectSample();
    }

    void StopSampling(bool completed)
    {
        timer_.stop();
        startButton_->setEnabled(true);
        stopButton_->setEnabled(false);
        if (!completed) {
            log_->appendPlainText("Sampling stopped by user. Analysis skipped unless at least 5 samples were collected.");
            if (samples_.size() < 5) return;
        }
        RunAnalysis();
    }

    void CollectSample()
    {
        if (sampleIndex_ >= targetSamples_) {
            StopSampling(true);
            return;
        }
        const auto sample = provider_.ReadSample(gpuCombo_->currentIndex(), sampleIndex_);
        samples_.push_back(sample);
        const int row = table_->rowCount();
        table_->insertRow(row);
        const QStringList values = {
            QString::number(row + 1),
            QString::number(sample.temperatureCelsius, 'f', 1),
            QString::number(sample.fanPercent, 'f', 1),
            QString::number(sample.powerWatts, 'f', 1),
            QString::number(sample.gpuUtilizationPercent, 'f', 1),
            QString::number(sample.memoryUtilizationPercent, 'f', 1),
            QString::number(sample.throttleReasons)
        };
        for (int column = 0; column < values.size(); ++column) table_->setItem(row, column, new QTableWidgetItem(values[column]));
        ++sampleIndex_;
        progress_->setValue(static_cast<int>(100.0 * sampleIndex_ / targetSamples_));
        if (sampleIndex_ >= targetSamples_) StopSampling(true);
    }

    void RunAnalysis()
    {
        if (samples_.size() < 5) {
            log_->appendPlainText("Analysis skipped: fewer than 5 samples.");
            return;
        }
        QString json;
        if (engineCombo_->currentText() == "Native C++ DLL") {
            QString error;
            if (!AnalyzeWithDll(samples_, json, error)) {
                log_->appendPlainText("Native DLL analysis failed: " + error);
                return;
            }
        } else if (engineCombo_->currentText() == "Wolfram Engine Calculation Engine") {
            auto result = MathematicaAnalysisEngine{}.Analyze(samples_);
            json = QString::fromStdString(result.rawJson);
        } else {
            auto result = MockAnalysisEngine{}.Analyze(samples_);
            json = QString::fromStdString(result.rawJson);
        }
        const auto doc = QJsonDocument::fromJson(json.toUtf8());
        analysis_->setHtml(FormatAnalysisHtml(doc, engineCombo_->currentText(), json));
        log_->appendPlainText("Analysis complete. Formatted result dashboard rendered from calculation JSON.");
    }

    GpuTelemetryProvider provider_;
    QComboBox* gpuCombo_ = nullptr;
    QSpinBox* durationSpin_ = nullptr;
    QComboBox* engineCombo_ = nullptr;
    QPushButton* startButton_ = nullptr;
    QPushButton* stopButton_ = nullptr;
    QProgressBar* progress_ = nullptr;
    QTableWidget* table_ = nullptr;
    QTextEdit* analysis_ = nullptr;
    QPlainTextEdit* log_ = nullptr;
    QTimer timer_;
    std::vector<GpuTelemetrySample> samples_;
    int sampleIndex_ = 0;
    int targetSamples_ = 30;
};

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    if (QCoreApplication::arguments().contains("--wolfram-engine-self-test") ||
        QCoreApplication::arguments().contains("--mathematica-self-test")) return RunWolframEngineSelfTest();
    if (QCoreApplication::arguments().contains("--self-test")) return RunSelfTest();
    DashboardWindow window;
    window.show();
    return app.exec();
}
