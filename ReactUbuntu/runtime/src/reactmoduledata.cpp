
#include <QObject>
#include <QMetaObject>

#include <QDebug>

#include "reactmoduleinterface.h"
#include "reactmoduledata.h"
#include "reactmodulemethod.h"


namespace
{
static int nextModuleId = 1;
// TODO: sort out all the issues around methodsToExport

QList<ReactModuleMethod*> buildMethodList(QObject* moduleImpl)
{
  const QMetaObject* metaObject = moduleImpl->metaObject();
  const int methodCount = metaObject->methodCount();

  QList<ReactModuleMethod*> methods;

  // from methodsToExport
  ReactModuleInterface* rmi = qobject_cast<ReactModuleInterface*>(moduleImpl);
  methods = rmi->methodsToExport();

  for (int i = metaObject->methodOffset(); i < methodCount; ++i) {
    QMetaMethod m = metaObject->method(i);
    if (m.methodType() == QMetaMethod::Method)
      methods << new ReactModuleMethod(metaObject->method(i),
                                       [moduleImpl](QVariantList&) {
                                         return moduleImpl;
                                       });
  }

  return methods;
}

QVariantMap buildConstantMap(QObject* moduleImpl)
{
  const QMetaObject* metaObject = moduleImpl->metaObject();
  const int propertyCount = metaObject->propertyCount();

  QVariantMap constants;

  // from constantsToExport
  ReactModuleInterface* rmi = qobject_cast<ReactModuleInterface*>(moduleImpl);
  constants = rmi->constantsToExport();

  // CONSTANT properties (limited usage?) - overrides values from constantsToExport
  for (int i = metaObject->propertyOffset(); i < propertyCount; ++i) {
    QMetaProperty p = metaObject->property(i);
    if (p.isConstant())
      constants.insert(p.name(), p.read(moduleImpl));
  }

  return constants;
}
}

class ReactModuleDataPrivate
{
public:
  int id;
  QObject* moduleImpl;
  QVariantMap constants;
  QList<ReactModuleMethod*> methods;
};

ReactModuleData::ReactModuleData(QObject* moduleImpl)
  : d_ptr(new ReactModuleDataPrivate)
{
  Q_D(ReactModuleData);
  d->id = nextModuleId++;
  d->moduleImpl = moduleImpl;
  d->constants = buildConstantMap(moduleImpl);
  d->methods = buildMethodList(moduleImpl);
}

ReactModuleData::~ReactModuleData()
{
  Q_D(ReactModuleData);
  d->moduleImpl->deleteLater();
}

int ReactModuleData::id() const
{
  return d_func()->id;
}

QString ReactModuleData::name() const
{
  return qobject_cast<ReactModuleInterface*>(d_func()->moduleImpl)->moduleName();
}

QVariant ReactModuleData::info() const
{
  Q_D(const ReactModuleData);

  QVariantMap config;
  config.insert("moduleID", d->id);

  if (!d->constants.isEmpty())
    config.insert("constants", d->constants);

  QVariantMap methodConfig;
  for (int i = 0; i < d->methods.size(); ++i) {
    methodConfig.insert(d->methods.at(i)->name(),
                        QVariantMap{{"methodID", i}, {"type", d->methods.at(i)->type()}});
  }
  if (!methodConfig.isEmpty())
    config.insert("methods", methodConfig);

  return config;
}

ReactModuleMethod* ReactModuleData::method(int id) const
{
  // assert bounds
  return d_func()->methods.value(id);
}

ReactViewManager* ReactModuleData::viewManager() const
{
  return qobject_cast<ReactModuleInterface*>(d_func()->moduleImpl)->viewManager();
}
