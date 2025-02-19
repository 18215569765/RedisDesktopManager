#include "qmlutils.h"
#include <qredisclient/utils/text.h>
#include <qtextdocumentfragment.h>
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QScreen>
#include <QtCharts/QDateTimeAxis>
#include <QtConcurrent>
#include <QUrl>

#include "apputils.h"
#include "jsonutils.h"
#include "qcompress.h"
#include "value-editor/largetextmodel.h"

#define MAX_CHART_DATA_POINTS 1000

bool QmlUtils::isBinaryString(const QVariant &value) {
  if (!value.canConvert(QVariant::ByteArray)) {
    return false;
  }
  QByteArray val = value.toByteArray();   

  return isBinary(val);
}

long QmlUtils::binaryStringLength(const QVariant &value) {
  if (!value.canConvert(QVariant::ByteArray)) {
    return -1;
  }
  QByteArray val = value.toByteArray();
  return val.size();
}

QVariant QmlUtils::b64toByteArray(const QVariant &value)
{
    if (!value.canConvert(QVariant::String)) {
      return -1;
    }

    return QVariant(QByteArray::fromBase64(value.toString().toUtf8()));
}

QByteArray QmlUtils::minifyJSON(const QVariant &value)
{
    if (!value.canConvert(QVariant::ByteArray)) {
      return QByteArray();
    }

    QByteArray val = value.toByteArray();

    return JSONUtils::minifyJSON(val);
}

QByteArray QmlUtils::prettyPrintJSON(const QVariant &value)
{
  if (!value.canConvert(QVariant::ByteArray)) {
    return QByteArray();
  }

  QByteArray val = value.toByteArray();
  return JSONUtils::prettyPrintJSON(val);
}

bool QmlUtils::isJSON(const QVariant &value)
{
    if (!value.canConvert(QVariant::ByteArray)) {
      return false;
    }

    QByteArray val = value.toByteArray();
    return JSONUtils::isJSON(val);
}

QVariant QmlUtils::decompress(const QVariant &value, unsigned alg) {
  if (!value.canConvert(QVariant::ByteArray)) {
    return 0;
  }

  return qcompress::decompress(value.toByteArray(), alg);
}

QVariant QmlUtils::compress(const QVariant &value, unsigned alg) {
  return qcompress::compress(value.toByteArray(), alg);
}

unsigned QmlUtils::isCompressed(const QVariant &value) {
  if (!value.canConvert(QVariant::ByteArray)) {
    return 0;
  }

  return qcompress::guessFormat(value.toByteArray());
}

QString QmlUtils::compressionAlgName(unsigned alg) {
    return qcompress::nameOf(alg);
}

QVariant QmlUtils::compressionMethodsNoMagic()
{
    QVariantList methodsWithoutMagicHeaders = {
        qcompress::UNKNOWN,
        qcompress::BROTLI,
        qcompress::LZ4_RAW,

        //NOTE(u_glide): Should be always last in this list
        // QML side use it for conditions
        qcompress::GZIP_PHP,
    };

    return methodsWithoutMagicHeaders;
}

QString QmlUtils::humanSize(long size) { return humanReadableSize(size); }

QVariant QmlUtils::valueToBinary(const QVariant &value) {
  if (!value.canConvert(QVariant::ByteArray)) {
    return QVariant();
  }

  QByteArray val = value.toByteArray();
  QVariantList list;

  for (int index = 0; index < val.length(); ++index) {
    list.append(QVariant((unsigned char)val.at(index)));
  }
  return QVariant(list);
}

QVariant QmlUtils::binaryListToValue(const QVariantList &binaryList) {
  QByteArray value;
  foreach (QVariant v, binaryList) { value.append((unsigned char)v.toInt()); }
  return value;
}

QVariant QmlUtils::printable(const QVariant &value, bool htmlEscaped, int maxLength) {
  if (!value.canConvert(QVariant::ByteArray)) {
    return QVariant();
  }

  QByteArray val = value.toByteArray();

  if (maxLength > 0 && val.size() > maxLength) {
    val.truncate(maxLength);
  }

  if (htmlEscaped) {
    return printableString(val).toHtmlEscaped();
  } else {
    return printableString(val);
  }
}

QVariant QmlUtils::printableToValue(const QVariant &printable) {
  if (!printable.canConvert(QVariant::String)) {
    return QVariant();
  }
  QString val = printable.toString();
  return printableStringToBinary(val);
}

QVariant QmlUtils::toUtf(const QVariant &value) {
  if (!value.canConvert(QVariant::ByteArray)) {
    return QVariant();
  }
  QByteArray val = value.toByteArray();
  QString result = QString::fromUtf8(val.constData(), val.size());
  return QVariant(result);
}

QString QmlUtils::getNativePath(const QString &path) {
  return QDir::toNativeSeparators(path);
}

QString QmlUtils::getPathFromUrl(const QUrl &url) {
  return url.isLocalFile() ? url.toLocalFile() : url.path();
}

QString QmlUtils::getUrlFromPath(const QString &path) {
  return QUrl::fromLocalFile(path).toString();
}

QString QmlUtils::getDir(const QString &path) {
  return QFileInfo(path).absoluteDir().absolutePath();
}

bool QmlUtils::fileExists(const QString &path) {
  return QFileInfo::exists(path);
}

void QmlUtils::copyToClipboard(const QString &text) {
  QClipboard *cb = QApplication::clipboard();

  if (!cb) return;

  cb->clear();
  cb->setText(text);
}

bool QmlUtils::saveToFile(const QVariant &value, const QString &path) {
  if (!value.canConvert(QVariant::ByteArray)) {
    return false;
  }

  QtConcurrent::run([value, path]() {
    QByteArray val = value.toByteArray();

    QFile outputFile(path);
    if (outputFile.open(QIODevice::WriteOnly)) {
      QDataStream outStream(&outputFile);
      outStream.writeRawData(val, val.size());
      outputFile.close();
      return true;
    }
    return false;
  });

  return true;
}

QtCharts::QDateTimeAxis *findDateTimeAxis(QtCharts::QXYSeries *series) {
  using namespace QtCharts;

  QList<QAbstractAxis *> axes = series->attachedAxes();

  QDateTimeAxis *ax = nullptr;

  for (QAbstractAxis *axis : axes) {
    if (axis->type() == QAbstractAxis::AxisTypeDateTime) {
      ax = qobject_cast<QDateTimeAxis *>(axis);
      return ax;
    }
  }

  return ax;
}

void QmlUtils::addNewValueToDynamicChart(QtCharts::QXYSeries *series,
                                         qreal value) {
  using namespace QtCharts;

  QDateTimeAxis *ax = findDateTimeAxis(series);

  if (!(ax && series)) {
      qWarning() << "Cannot add value to dynamic chart. Invalid pointers.";
      return;
  }

  int totalPoints = series->count();

  if (totalPoints == 0) {
    ax->setMin(QDateTime::currentDateTime());
  }

  bool dataNotChangedLastFivePoints = totalPoints > 10;

  for (int i = 1; dataNotChangedLastFivePoints && i < 6; i++) {
    if (value != series->at(totalPoints - i).y()) {
      dataNotChangedLastFivePoints = false;
      break;
    }
  }

  if (dataNotChangedLastFivePoints) {
    series->replace(totalPoints - 1, QDateTime::currentDateTime().toMSecsSinceEpoch(), value);
  } else {
    series->append(QDateTime::currentDateTime().toMSecsSinceEpoch(), value);
  }

  if (totalPoints > MAX_CHART_DATA_POINTS) {
      series->removePoints(0, totalPoints - MAX_CHART_DATA_POINTS);
      ax->setMin(QDateTime::fromMSecsSinceEpoch(series->at(0).x()));
  }

  if (series->attachedAxes().size() > 0) {
    ax->setMax(QDateTime::currentDateTime());
  }
}

QObject *QmlUtils::wrapLargeText(const QByteArray &text) {
  // NOTE(u_glide): Use 50Kb chunks by default
  int chunkSize = 50000;

  auto w = new ValueEditor::LargeTextWrappingModel(QString::fromUtf8(text),
                                                   chunkSize);
  w->setParent(this);
  return w;
}

void QmlUtils::deleteTextWrapper(QObject *w) {
  if (w && w->parent() == this) {
    w->deleteLater();
  }
}

QString QmlUtils::escapeHtmlEntities(const QString &t) {
  return t.toHtmlEscaped();
}

QString QmlUtils::standardKeyToString(QKeySequence::StandardKey key) {
  return QKeySequence(key).toString(QKeySequence::NativeText);
}

double QmlUtils::getScreenScaleFactor() {
    return QApplication::primaryScreen()->logicalDotsPerInch() / 96;
}

bool QmlUtils::isAppStoreBuild()
{
#ifdef RDM_APPSTORE
    return true;
#else
    return false;
#endif
}
