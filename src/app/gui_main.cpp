#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QPainter>
#include <QProcess>
#include <QPushButton>
#include <QResizeEvent>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <algorithm>
#include <memory>

class ImageView : public QGraphicsView {
public:
    explicit ImageView(const QString& placeholder, QWidget* parent = nullptr)
        : QGraphicsView(parent), placeholder_(placeholder) {
        setScene(new QGraphicsScene(this));
        setDragMode(QGraphicsView::ScrollHandDrag);
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        setBackgroundBrush(Qt::black);
        setFrameShape(QFrame::NoFrame);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    void showImage(const QPixmap& image) {
        scene()->clear();
        has_image_ = !image.isNull();
        if (has_image_) scene()->addPixmap(image);
        scene()->setSceneRect(image.rect());
        setBackgroundBrush(has_image_ ? QColor(238, 240, 243) : QColor(Qt::black));
        resetTransform();
        fitImage();
        viewport()->update();
    }

    void fitImage() {
        auto_fit_ = true;
        if (!scene()->sceneRect().isEmpty())
            fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QGraphicsView::paintEvent(event);
        if (!has_image_) {
            QPainter painter(viewport());
            painter.setPen(Qt::white);
            QFont font = painter.font();
            font.setPointSize(12);
            painter.setFont(font);
            painter.drawText(viewport()->rect(), Qt::AlignCenter, placeholder_);
        }
    }

    void resizeEvent(QResizeEvent* event) override {
        QGraphicsView::resizeEvent(event);
        if (auto_fit_) fitImage();
    }

    void wheelEvent(QWheelEvent* event) override {
        if (scene()->sceneRect().isEmpty()) return;
        const double factor = event->angleDelta().y() > 0 ? 1.2 : 1.0 / 1.2;
        const double next = transform().m11() * factor;
        if (next >= 0.01 && next <= 20.0) {
            auto_fit_ = false;
            scale(factor, factor);
        }
        event->accept();
    }

private:
    QString placeholder_;
    bool has_image_ = false;
    bool auto_fit_ = true;
};

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(const QString& initial_image = {}) {
        setWindowTitle(QStringLiteral("工业摄影测量技术作业2"));
        resize(1100, 780);
        setMinimumSize(640, 480);

        auto* central = new QWidget(this);
        auto* layout = new QVBoxLayout(central);
        layout->setContentsMargins(16, 16, 16, 12);
        layout->setSpacing(12);
        auto* tools = new QHBoxLayout;
        open_ = new QPushButton(QStringLiteral("打开图片"));
        run_ = new QPushButton(QStringLiteral("识别并编号"));
        run_->setObjectName("runButton");
        save_ = new QPushButton(QStringLiteral("导出结果"));
        save_->setObjectName("saveButton");


        tools->addWidget(open_);
        tools->addWidget(run_);
        tools->addWidget(save_);
        tools->addStretch();


        layout->addLayout(tools);
        auto* comparison = new QSplitter(Qt::Horizontal);
        original_view_ = new ImageView(QStringLiteral("输入图像"));
        original_view_->setObjectName("originalView");
        numbered_view_ = new ImageView(QStringLiteral("输出图像"));
        numbered_view_->setObjectName("numberedView");
        const auto add_panel = [comparison](const QString& title, ImageView* view) {
            auto* panel = new QWidget;
            auto* panel_layout = new QVBoxLayout(panel);
            panel_layout->setContentsMargins(0, 0, 0, 0);
            auto* label = new QLabel(title);
            label->setAlignment(Qt::AlignCenter);
            panel_layout->addWidget(label);
            panel_layout->addWidget(view, 1);
            comparison->addWidget(panel);
        };
        add_panel(QStringLiteral("原图"), original_view_);
        add_panel(QStringLiteral("编号图"), numbered_view_);
        comparison->setChildrenCollapsible(false);
        comparison->setStretchFactor(0, 1);
        comparison->setStretchFactor(1, 1);
        layout->addWidget(comparison, 1);
        setCentralWidget(central);
        run_->setEnabled(false);
        save_->setEnabled(false);

        connect(open_, &QPushButton::clicked, this, [this] {
            const QString path = QFileDialog::getOpenFileName(
                this, QStringLiteral("打开靶标图片"),
                input_.isEmpty() ? QDir::currentPath() : QFileInfo(input_).absolutePath(),
                QStringLiteral("图片 (*.bmp *.png *.jpg *.jpeg *.tif *.tiff);;所有文件 (*)"));
            if (!path.isEmpty()) loadImage(path);
        });
        connect(run_, &QPushButton::clicked, this, [this] { startProcessing(); });
        connect(save_, &QPushButton::clicked, this, [this] { exportResults(); });

        connect(&process_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this](int code, QProcess::ExitStatus exit_status) {
            setBusy(false);
            const QString output = QString::fromUtf8(process_.readAllStandardOutput()).trimmed();
            const QString errors = QString::fromUtf8(process_.readAllStandardError()).trimmed();
            if (exit_status != QProcess::NormalExit || code != 0) {
                fail(QStringLiteral("识别失败\n%1\n%2").arg(output, errors));
                return;
            }
            const QPixmap annotated(result_dir_ + "/annotated.png");
            if (annotated.isNull() || !QFileInfo::exists(result_dir_ + "/binary.png") ||
                !QFileInfo::exists(result_dir_ + "/coordinates.csv")) {
                fail(QStringLiteral("结果文件缺失或无法读取。"));
                return;
            }
            numbered_view_->showImage(annotated);
            save_->setEnabled(true);
        });
        connect(&process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
            if (error == QProcess::FailedToStart) {
                setBusy(false);
                fail(QStringLiteral("无法启动识别程序：%1").arg(process_.errorString()));
            }
        });
        if (!initial_image.isEmpty()) loadImage(initial_image);
    }

    ~MainWindow() override {
        // 先结束子进程，再销毁其正在写入的临时目录。
        if (process_.state() != QProcess::NotRunning) {
            process_.kill();
            process_.waitForFinished();
        }
    }

private:
    void loadImage(const QString& path) {
        const QPixmap image(path);
        if (image.isNull()) {
            fail(QStringLiteral("无法读取图片：%1").arg(path));
            return;
        }
        input_ = QFileInfo(path).absoluteFilePath();
        numbered_view_->showImage(QPixmap());
        temporary_.reset();
        result_dir_.clear();
        original_view_->showImage(image);
        run_->setEnabled(true);
        save_->setEnabled(false);
        save_->setText(QStringLiteral("导出结果"));
        setWindowTitle(QStringLiteral("工业摄影测量技术作业2"));

    }

    void setBusy(bool busy) {
        open_->setEnabled(!busy);
        run_->setEnabled(!busy && !input_.isEmpty());
        save_->setEnabled(false);
        run_->setText(busy ? QStringLiteral("正在识别…") : QStringLiteral("识别并编号"));
    }

    void startProcessing() {
        const QString backend = QCoreApplication::applicationDirPath() + "/pipeline";
        if (!QFileInfo(backend).isExecutable()) {
            fail(QStringLiteral("找不到识别程序：%1\n请先构建 pipeline 目标。").arg(backend));
            return;
        }
        numbered_view_->showImage(QPixmap());
        save_->setText(QStringLiteral("导出结果"));
        temporary_ = std::make_unique<QTemporaryDir>();
        save_->setEnabled(false);
        if (!temporary_->isValid()) {
            fail(QStringLiteral("无法创建临时结果目录。"));
            return;
        }
        result_dir_ = temporary_->path() + "/" + QFileInfo(input_).completeBaseName();
        setBusy(true);

        process_.start(backend, {input_, temporary_->path()});
    }

    void exportResults() {
        const QString root = QFileDialog::getExistingDirectory(this, QStringLiteral("选择导出根目录"));
        if (root.isEmpty()) return;
        const QString destination = QDir(root).filePath(QFileInfo(input_).completeBaseName());
        QStringList names;
        for (const QString& name : {QStringLiteral("annotated.png"),
                                    QStringLiteral("binary.png"),
                                    QStringLiteral("coordinates.csv")}) {
            if (QFileInfo::exists(result_dir_ + "/" + name)) names << name;
        }
        const bool overwrites = std::any_of(names.begin(), names.end(), [&](const QString& name) {
            return QFileInfo::exists(destination + "/" + name);
        });
        if (overwrites && QMessageBox::question(this, QStringLiteral("覆盖确认"),
                QStringLiteral("目标目录已有同名结果，是否覆盖？\n%1").arg(destination),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) return;
        if (!QDir().mkpath(destination)) {
            fail(QStringLiteral("无法创建导出目录：%1").arg(destination));
            return;
        }
        for (const QString& name : names) {
            const QString target = destination + "/" + name;
            QFile source(result_dir_ + "/" + name);
            QSaveFile output(target);
            bool copied = source.open(QIODevice::ReadOnly) && output.open(QIODevice::WriteOnly);
            while (copied && !source.atEnd()) {
                const QByteArray chunk = source.read(1024 * 1024);
                copied = source.error() == QFileDevice::NoError && output.write(chunk) == chunk.size();
            }
            if (!copied || !output.commit()) {
                fail(QStringLiteral("导出失败：%1\n请检查写入权限；部分文件可能已导出。").arg(target));
                return;
            }
        }
        save_->setText(QStringLiteral("已导出"));
    }

    void fail(const QString& message) {

        QMessageBox::warning(this, QStringLiteral("提示"), message);
    }

    QProcess process_;
    std::unique_ptr<QTemporaryDir> temporary_;
    QString input_;
    QString result_dir_;
    ImageView* original_view_ = nullptr;
    ImageView* numbered_view_ = nullptr;
    QPushButton* open_ = nullptr;
    QPushButton* run_ = nullptr;
    QPushButton* save_ = nullptr;
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    MainWindow window(app.arguments().value(1));
    window.show();
    return app.exec();
}
