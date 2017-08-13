#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQmlContext>

#include "src/generator.h"
#include "src/measure.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QQuickStyle::setStyle("Material");

    Generator g;
    Measure *m;
    m = new Measure(nullptr);

    QQmlApplicationEngine engine;


    //qmlRegisterType<Complex>();
    //Q_DECLARE_METATYPE(QQmlListProperty<Complex>)

    engine.rootContext()->setContextProperty("generatorModel", &g);
    engine.rootContext()->setContextProperty("measureModel", m);
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));


    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
