#include <QApplication>
#include <QQuickView>
#include <QtQml>
#include <QDebug>

#include <string.h>

#include "govalue.h"
#include "govaluetype.h"
#include "capi.h"

void newGuiApplication()
{
    static char empty[1] = {0};
    static char *argv[] = {empty};
    static int argc = 1;
    new QGuiApplication(argc, argv);

    // The event should never die.
    qApp->setQuitOnLastWindowClosed(false);
}

void applicationExec()
{
    qApp->exec();
}

void applicationFlushAll()
{
    qApp->processEvents();
}

void *currentThread()
{
    return QThread::currentThread();
}

void *appThread()
{
    return QCoreApplication::instance()->thread();
}

QQmlEngine_ *newEngine(QObject_ *parent)
{
    return new QQmlEngine(reinterpret_cast<QObject *>(parent));
}

QQmlContext_ *engineRootContext(QQmlEngine_ *engine)
{
    return reinterpret_cast<QQmlEngine *>(engine)->rootContext();
}

void engineSetContextForObject(QQmlEngine_ *engine, QObject_ *object)
{
    QQmlEngine *qengine = reinterpret_cast<QQmlEngine *>(engine);
    QObject *qobject = reinterpret_cast<QObject *>(object);

    QQmlEngine::setContextForObject(qobject, qengine->rootContext());
}

void engineSetOwnershipCPP(QQmlEngine_ *engine, QObject_ *object)
{
    QQmlEngine *qengine = reinterpret_cast<QQmlEngine *>(engine);
    QObject *qobject = reinterpret_cast<QObject *>(object);

    qengine->setObjectOwnership(qobject, QQmlEngine::CppOwnership);
}

void engineSetOwnershipJS(QQmlEngine_ *engine, QObject_ *object)
{
    QQmlEngine *qengine = reinterpret_cast<QQmlEngine *>(engine);
    QObject *qobject = reinterpret_cast<QObject *>(object);

    qengine->setObjectOwnership(qobject, QQmlEngine::JavaScriptOwnership);
}

QQmlComponent_ *newComponent(QQmlEngine_ *engine, QObject_ *parent)
{
    QQmlEngine *qengine = reinterpret_cast<QQmlEngine *>(engine);
    //QObject *qparent = reinterpret_cast<QObject *>(parent);
    return new QQmlComponent(qengine);
}

void componentSetData(QQmlComponent_ *component, const char *data, int dataLen, const char *url, int urlLen)
{
    QByteArray qdata(data, dataLen);
    QByteArray qurl(url, urlLen);
    QString qsurl = QString::fromUtf8(qurl);
    reinterpret_cast<QQmlComponent *>(component)->setData(qdata, qsurl);
}

static char *local_strdup(const char *str)
{
    char *strcopy = 0;
    if (str) {
        size_t len = strlen(str) + 1;
        strcopy = (char *)malloc(len);
        memcpy(strcopy, str, len);
    }
    return strcopy;
}

char *componentErrorString(QQmlComponent_ *component)
{
    QQmlComponent *qcomponent = reinterpret_cast<QQmlComponent *>(component);
    if (qcomponent->isReady()) {
        return NULL;
    }
    if (qcomponent->isError()) {
        QByteArray ba = qcomponent->errorString().toUtf8();
        return local_strdup(ba.constData());
    }
    return local_strdup("component is not ready (why!?)");
}

QObject_ *componentCreate(QQmlComponent_ *component, QQmlContext_ *context)
{
    QQmlComponent *qcomponent = reinterpret_cast<QQmlComponent *>(component);
    QQmlContext *qcontext = reinterpret_cast<QQmlContext *>(context);

    if (!qcontext) {
        qcontext = qmlContext(qcomponent);
    }
    return qcomponent->create(qcontext);
}

QQuickView_ *componentCreateView(QQmlComponent_ *component, QQmlContext_ *context)
{
    QQmlComponent *qcomponent = reinterpret_cast<QQmlComponent *>(component);
    QQmlContext *qcontext = reinterpret_cast<QQmlContext *>(context);

    if (!qcontext) {
        qcontext = qmlContext(qcomponent);
    }
    QObject *instance = qcomponent->create(qcontext);
    QQuickView *view = new QQuickView(qmlEngine(qcomponent), 0);
    view->setContent(qcomponent->url(), qcomponent, instance);
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    return view;
}

void viewShow(QQuickView_ *view)
{
    reinterpret_cast<QQuickView *>(view)->show();
}

void viewHide(QQuickView_ *view)
{
    reinterpret_cast<QQuickView *>(view)->hide();
}

void viewConnectHidden(QQuickView_ *view)
{
    QQuickView *qview = reinterpret_cast<QQuickView *>(view);
    QObject::connect(qview, &QWindow::visibleChanged, [=](bool visible){
        if (!visible) {
            hookWindowHidden(view);
        }
    });
}

QObject_ *viewRootObject(QQuickView_ *view)
{
    QQuickView *qview = reinterpret_cast<QQuickView *>(view);
    return qview->rootObject();
}

QImage_ *viewGrabWindow(QQuickView_ *view)
{
    QQuickView *qview = reinterpret_cast<QQuickView *>(view);
    QImage *image = new QImage;
    *image = qview->grabWindow().convertToFormat(QImage::Format_ARGB32_Premultiplied);
    return image;
}

void delImage(QImage_ *image)
{
    delete reinterpret_cast<QImage *>(image);
}

void imageSize(QImage_ *image, int *width, int *height)
{
    QImage *qimage = reinterpret_cast<QImage *>(image);
    *width = qimage->width();
    *height = qimage->height();
}

const unsigned char *imageBits(QImage_ *image)
{
    QImage *qimage = reinterpret_cast<QImage *>(image);
    return qimage->constBits();
}

void contextSetObject(QQmlContext_ *context, QObject_ *value)
{
    QQmlContext *qcontext = reinterpret_cast<QQmlContext *>(context);
    QObject *qvalue = reinterpret_cast<QObject *>(value);

    // Give qvalue an engine reference if it doesn't yet have one.
    if (!qmlEngine(qvalue)) {
        QQmlEngine::setContextForObject(qvalue, qcontext->engine()->rootContext());
    }

    qcontext->setContextObject(qvalue);
}

void contextSetProperty(QQmlContext_ *context, QString_ *name, DataValue *value)
{
    const QString *qname = reinterpret_cast<QString *>(name);
    QQmlContext *qcontext = reinterpret_cast<QQmlContext *>(context);

    QVariant var;
    unpackDataValue(value, &var);

    // Give qvalue an engine reference if it doesn't yet have one .
    QObject *obj = var.value<QObject *>();
    if (obj && !qmlEngine(obj)) {
        QQmlEngine::setContextForObject(obj, qcontext);
    }

    qcontext->setContextProperty(*qname, var);
}

void contextGetProperty(QQmlContext_ *context, QString_ *name, DataValue *result)
{
    QQmlContext *qcontext = reinterpret_cast<QQmlContext *>(context);
    const QString *qname = reinterpret_cast<QString *>(name);

    QVariant var = qcontext->contextProperty(*qname);
    packDataValue(&var, result);
}

void delObject(QObject_ *object)
{
    delete reinterpret_cast<QObject *>(object);
}

void delObjectLater(QObject_ *object)
{
    reinterpret_cast<QObject *>(object)->deleteLater();
}

int objectGetProperty(QObject_ *object, const char *name, DataValue *result)
{
    QObject *qobject = reinterpret_cast<QObject *>(object);
    
    QVariant var = qobject->property(name);
    packDataValue(&var, result);

    if (!var.isValid() && qobject->metaObject()->indexOfProperty(name) == -1) {
            // TODO May have to check the dynamic property names too.
            return 0;
    }
    return 1;
}

void objectSetProperty(QObject_ *object, const char *name, DataValue *value)
{
    QObject *qobject = reinterpret_cast<QObject *>(object);
    QVariant var;
    unpackDataValue(value, &var);

    // Give qvalue an engine reference if it doesn't yet have one.
    QObject *obj = var.value<QObject *>();
    if (obj && !qmlEngine(obj)) {
        QQmlContext *context = qmlContext(qobject);
        if (context) {
            QQmlEngine::setContextForObject(obj, context);
        }
    }

    qobject->setProperty(name, var);
}

void objectInvoke(QObject_ *object, const char *method, DataValue *resultdv, DataValue *paramsdv, int paramsLen)
{
    QObject *qobject = reinterpret_cast<QObject *>(object);

    QVariant result;
    QVariant param[MaximumParamCount-1];
    QGenericArgument arg[MaximumParamCount-1];
    for (int i = 0; i < paramsLen; i++) {
        unpackDataValue(&paramsdv[i], &param[i]);
        arg[i] = Q_ARG(QVariant, param[i]);
    }
    if (paramsLen > 10) {
        qFatal("fix the parameter dispatching");
    }
    bool ok = QMetaObject::invokeMethod(qobject, method, Qt::DirectConnection, 
            Q_RETURN_ARG(QVariant, result),
            arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7], arg[8], arg[9]);
    if (!ok) {
        // TODO Find out how to tell if a result is available or not without calling it twice.
        ok = QMetaObject::invokeMethod(qobject, method, Qt::DirectConnection, 
            arg[0], arg[1], arg[2], arg[3], arg[4], arg[5], arg[6], arg[7], arg[8], arg[9]);
    }
    packDataValue(&result, resultdv);
}

void objectFindChild(QObject_ *object, QString_ *name, DataValue *resultdv)
{
    QObject *qobject = reinterpret_cast<QObject *>(object);
    QString *qname = reinterpret_cast<QString *>(name);
    
    QVariant var;
    QObject *result = qobject->findChild<QObject *>(*qname);
    if (result) {
        var.setValue(result);
    }
    packDataValue(&var, resultdv);
}

void objectSetParent(QObject_ *object, QObject_ *parent)
{
    QObject *qobject = reinterpret_cast<QObject *>(object);
    QObject *qparent = reinterpret_cast<QObject *>(parent);

    qobject->setParent(qparent);
}

QQmlContext_ *objectContext(QObject_ *object)
{
    return qmlContext(reinterpret_cast<QObject *>(object));
}

int objectIsComponent(QObject_ *object)
{
    QObject *qobject = reinterpret_cast<QObject *>(object);
    return dynamic_cast<QQmlComponent *>(qobject) ? 1 : 0;
}

QString_ *newString(const char *data, int len)
{
    // This will copy data only once.
    QByteArray ba = QByteArray::fromRawData(data, len);
    return new QString(ba);
}

void delString(QString_ *s)
{
    delete reinterpret_cast<QString *>(s);
}

GoValue_ *newGoValue(GoAddr *addr, GoTypeInfo *typeInfo, QObject_ *parent)
{
    QObject *qparent = reinterpret_cast<QObject *>(parent);
    return new GoValue(addr, typeInfo, qparent);
}

void goValueActivate(GoValue_ *value, GoTypeInfo *typeInfo, int addrOffset)
{
    GoMemberInfo *fieldInfo = typeInfo->fields;
    for (int i = 0; i < typeInfo->fieldsLen; i++) {
        if (fieldInfo->addrOffset == addrOffset) {
            reinterpret_cast<GoValue *>(value)->activate(fieldInfo->metaIndex);
            return;
        }
        fieldInfo++;
    }

    // TODO Return an error; probably an unexported field.
}

template<int N>
void registerSingletonN(char *location, int major, int minor, char *name, GoTypeInfo *info, GoTypeSpec_ *spec) {
    GoValueType<N>::init(info, spec);
    qmlRegisterSingletonType< GoValueType<N> >(location, major, minor, name, [](QQmlEngine *qmlEngine, QJSEngine *jsEngine) -> QObject* {
        QObject *singleton = new GoValueType<N>();
        QQmlEngine::setContextForObject(singleton, qmlEngine->rootContext());
        return singleton;
    });
}

void registerSingleton(char *location, int major, int minor, char *name, GoTypeInfo *info, GoTypeSpec_ *spec)
{
    // TODO Must increment the number and use different types per call.
    registerSingletonN<1>(location, major, minor, name, info, spec);
}

void registerType(char *location, int major, int minor, char *name, GoTypeInfo *info, GoTypeSpec_ *spec)
{
    // TODO Must increment the number and use different types per call.
    GoValueType<1>::init(info, spec);
    qmlRegisterType< GoValueType<1> >(location, major, minor, name);
}

void unpackDataValue(DataValue *value, QVariant_ *var)
{
    QVariant *qvar = reinterpret_cast<QVariant *>(var);
    switch (value->dataType) {
    case DTString:
        *qvar = QString::fromUtf8(*(char **)value->data, value->len);
        break;
    case DTBool:
        *qvar = bool(*(char *)(value->data) != 0);
        break;
    case DTInt64:
        *qvar = *(qint64*)(value->data);
        break;
    case DTInt32:
        *qvar = *(qint32*)(value->data);
        break;
    case DTFloat64:
        *qvar = *(double*)(value->data);
        break;
    case DTFloat32:
        *qvar = *(float*)(value->data);
        break;
    case DTList:
        *qvar = **(QVariantList**)(value->data);
        delete *(QVariantList**)(value->data);
        break;
    case DTObject:
        qvar->setValue(*(QObject**)(value->data));
        break;
    case DTInvalid:
        // null would be more natural, but an invalid variant means
        // it has proper semantics when dealing with non-qml qt code.
        //qvar->setValue(QJSValue(QJSValue::NullValue));
        qvar->clear();
        break;
    default:
        qFatal("Unsupported data type: %d", value->dataType);
        break;
    }
}

void packDataValue(QVariant_ *var, DataValue *value)
{
    QVariant *qvar = reinterpret_cast<QVariant *>(var);

    // Some assumptions are made below regarding the size of types.
    // There's apparently no better way to handle this since that's
    // how the types with well defined sizes (qint64) are mapped to
    // meta-types (QMetaType::LongLong).
    switch ((int)qvar->type()) {
    case QVariant::Invalid:
        value->dataType = DTInvalid;
        break;
    case QMetaType::QString:
        {
            value->dataType = DTString;
            QByteArray ba = qvar->toByteArray();
            *(char**)(value->data) = local_strdup(ba.constData());
            value->len = ba.size();
            break;
        }
    case QMetaType::Bool:
        value->dataType = DTBool;
        *(qint8*)(value->data) = (qint8)qvar->toInt();
        break;
    case QMetaType::LongLong:
        value->dataType = DTInt64;
        *(qint64*)(value->data) = qvar->toLongLong();
        break;
    case QMetaType::Int:
        value->dataType = DTInt32;
        *(qint32*)(value->data) = qvar->toInt();
        break;
    case QMetaType::Double:
        value->dataType = DTFloat64;
        *(double*)(value->data) = qvar->toDouble();
        break;
    case QMetaType::Float:
        value->dataType = DTFloat32;
        *(float*)(value->data) = qvar->toFloat();
        break;
    case QMetaType::QObjectStar:
        {
            QObject *qobject = qvar->value<QObject *>();
            GoValue *govalue = dynamic_cast<GoValue *>(qobject);
            if (govalue) {
                value->dataType = DTGoAddr;
                *(void **)(value->data) = govalue->addr();
            } else {
                value->dataType = DTObject;
                *(void **)(value->data) = qobject;
            }
            break;
        }
        // fallthrough
    default:
        qFatal("Unsupported variant type: %d", qvar->type());
        break;
    }
}

QVariantList_ *newVariantList(DataValue *list, int len)
{
    QVariantList *vlist = new QVariantList();
    vlist->reserve(len);
    for (int i = 0; i < len; i++) {
        QVariant var;
        unpackDataValue(&list[i], &var);
        vlist->append(var);
    }
    return vlist;
}

void internalLogHandler(QtMsgType severity, const QMessageLogContext &context, const QString &text)
{
    QByteArray textba = text.toUtf8();
    LogMessage message = {severity, textba.constData(), textba.size(), context.file, (int)strlen(context.file), context.line};
    hookLogHandler(&message);
}

void installLogHandler()
{
    qInstallMessageHandler(internalLogHandler);
}

// vim:ts=4:sw=4:et:ft=cpp
