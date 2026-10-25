#pragma once
#include "node_list_model.hpp"
#include <QAudioOutput>
#include <QElapsedTimer>
#include <QMediaPlayer>
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <studio/core/editor.hpp>
#include <studio/storage/project_store.hpp>

class EditorController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *nodes READ nodeModel CONSTANT)
    Q_PROPERTY(QVariantMap selected READ selected NOTIFY selectedChanged)
    Q_PROPERTY(QVariantMap camera READ camera NOTIFY cameraChanged)
    Q_PROPERTY(QString title READ title NOTIFY documentChanged)
    Q_PROPERTY(QString selectedId READ selectedId NOTIFY documentChanged)
    Q_PROPERTY(int frame READ frame WRITE setFrame NOTIFY frameChanged)
    Q_PROPERTY(int duration READ duration NOTIFY documentChanged)
    Q_PROPERTY(double fps READ fps NOTIFY documentChanged)
    Q_PROPERTY(int canvasWidth READ canvasWidth NOTIFY documentChanged)
    Q_PROPERTY(int canvasHeight READ canvasHeight NOTIFY documentChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(bool dirty READ dirty NOTIFY documentChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY documentChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY documentChanged)
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)
    Q_PROPERTY(bool exporting READ exporting NOTIFY exportChanged)
    Q_PROPERTY(int exportProgress READ exportProgress NOTIFY exportChanged)
    Q_PROPERTY(bool recoveryAvailable READ recoveryAvailable NOTIFY documentChanged)
    Q_PROPERTY(QVariantList library READ library CONSTANT)
    Q_PROPERTY(QVariantList scenes READ scenes NOTIFY documentChanged)
    Q_PROPERTY(QVariantList shots READ shots NOTIFY documentChanged)
    Q_PROPERTY(QVariantList visualBeats READ visualBeats NOTIFY documentChanged)
    Q_PROPERTY(QVariantList waveform READ waveform NOTIFY documentChanged)
    Q_PROPERTY(bool hasAudio READ hasAudio NOTIFY documentChanged)
    Q_PROPERTY(bool producing READ producing NOTIFY productionChanged)
    Q_PROPERTY(int productionProgress READ productionProgress NOTIFY productionChanged)
    Q_PROPERTY(QUrl latestProduction READ latestProduction NOTIFY productionChanged)
    Q_PROPERTY(QString latestProductionLog READ latestProductionLog NOTIFY productionChanged)
  public:
    explicit EditorController(QObject *parent = nullptr, QString recoveryDirectory = {});
    const studio::Project &project() const {
        return editor_.project();
    }
    QVariantList nodes() const;
    QVariantList library() const;
    QVariantList scenes() const;
    QVariantList shots() const;
    QVariantList visualBeats() const;
    QVariantList waveform() const {
        return waveform_;
    }
    bool hasAudio() const {
        return !project().audioSource.empty();
    }
    bool producing() const {
        return production_.state() != QProcess::NotRunning;
    }
    int productionProgress() const {
        return productionProgress_;
    }
    QUrl latestProduction() const {
        return latestProduction_;
    }
    Q_INVOKABLE void importProduction(QUrl recipe);
    Q_INVOKABLE void importCarExample();
    Q_INVOKABLE void cancelProduction();
    Q_INVOKABLE bool openLogsFolder();
    QString latestProductionLog() const {
        return productionLogPath_;
    }
    QAbstractItemModel *nodeModel() {
        return &nodeModel_;
    }
    QVariantMap selected() const;
    QVariantMap camera() const;
    QString title() const;
    QString selectedId() const {
        return selectedId_;
    }
    int frame() const {
        return frame_;
    }
    int duration() const {
        return int(project().duration);
    }
    double fps() const {
        return project().fps.value();
    }
    int canvasWidth() const {
        return project().width;
    }
    int canvasHeight() const {
        return project().height;
    }
    bool playing() const {
        return playing_;
    }
    bool dirty() const;
    bool canUndo() const {
        return editor_.canUndo();
    }
    bool canRedo() const {
        return editor_.canRedo();
    }
    QString message() const {
        return message_;
    }
    bool exporting() const {
        return render_.state() != QProcess::NotRunning;
    }
    int exportProgress() const {
        return exportProgress_;
    }
    bool recoveryAvailable() const;
    Q_INVOKABLE void selectNode(QString id);
    Q_INVOKABLE void updateSelected(QString key, QVariant value);
    Q_INVOKABLE void beginGesture();
    Q_INVOKABLE void endGesture();
    Q_INVOKABLE void moveSelected(double x, double y);
    Q_INVOKABLE void moveClip(QString id, int start);
    Q_INVOKABLE void moveShotBoundary(QString rightShotId, int frame);
    Q_INVOKABLE void addNode(QString kind, QUrl source = {});
    Q_INVOKABLE void removeSelected();
    Q_INVOKABLE void groupSelected();
    Q_INVOKABLE void recordSelectedKeyframe();
    Q_INVOKABLE void setSelectedKeyframeEasing(QString easing);
    Q_INVOKABLE void setCameraKeyframeEasing(QString easing);
    Q_INVOKABLE void updateCamera(QString property, double value);
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void setFrame(int frame);
    Q_INVOKABLE void togglePlayback();
    Q_INVOKABLE bool save(QUrl path = {});
    Q_INVOKABLE bool open(QUrl path);
    QString projectPath() const {
        return path_;
    }
    bool insertLibraryNode(studio::Node node);
    bool insertLibraryNodes(std::vector<studio::Node> nodes);
    Q_INVOKABLE void newProject(bool demo = false);
    Q_INVOKABLE void recover();
    Q_INVOKABLE void exportVideo(QUrl output);
    Q_INVOKABLE void cancelExport();
    Q_INVOKABLE QString timecode(int frame) const;
    bool hasPath() const {
        return !path_.isEmpty();
    }
    Q_INVOKABLE bool savedBefore() const {
        return hasPath();
    }
  signals:
    void documentChanged();
    void selectedChanged();
    void cameraChanged();
    void frameChanged();
    void playingChanged();
    void messageChanged();
    void exportChanged();
    void productionChanged();
    void projectAvailable(QString path);

  private:
    void changed();
    void notify(QString message);
    void stop();
    void presentFrame(int frame);
    void autosave();
    void reset(studio::Project p, QString path = {}, QByteArray revision = {});
    void setupProduction();
    void startProductionLog(const QString &recipeFileName);
    void logProductionEvent(const QString &event, const QString &message = {}, int progress = -1);
    QString currentSceneId() const;
    QString currentShotId() const;
    studio::Editor editor_{studio::makeDemo()};
    QString selectedId_{"title"}, path_, message_{"Pronto para criar. Selecione um objeto para começar."};
    QByteArray savedDocument_, revision_, recoveryRevision_, renderBuffer_;
    QString recoveryPath_, renderSnapshot_, cancelPath_;
    QTimer playback_, autosave_;
    QElapsedTimer clock_;
    int frame_{60}, playStart_{0}, exportProgress_{0};
    bool playing_{false}, autosaveEnabled_{false};
    QProcess render_;
    NodeListModel nodeModel_{this};
    QMediaPlayer audioPlayer_;
    QAudioOutput audioOutput_;
    QVariantList waveform_;
    QProcess production_;
    QByteArray productionBuffer_, productionBaseline_;
    QString productionDirectory_, productionCancel_, productionLogPath_, diagnosticsDirectory_;
    int productionProgress_{0};
    QUrl latestProduction_;
};
