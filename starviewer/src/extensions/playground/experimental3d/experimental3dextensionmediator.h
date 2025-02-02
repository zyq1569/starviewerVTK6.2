#ifndef UDGEXPERIMENTAL3DEXTENSIONMEDIATOR_H
#define UDGEXPERIMENTAL3DEXTENSIONMEDIATOR_H

#include "extensionmediator.h"

#include "installextension.h"
#include "qexperimental3dextension.h"

namespace udg {

class Experimental3DExtensionMediator : public ExtensionMediator {

    Q_OBJECT

public:

    Experimental3DExtensionMediator(QObject *parent = 0);
    ~Experimental3DExtensionMediator();

    virtual DisplayableID getExtensionID() const;

    virtual bool initializeExtension(QWidget *extension, const ExtensionContext &extensionContext);
	//---20200919---add
	virtual void executionCommand(QWidget *extension, Volume* volume, void *data = NULL, int command = 0)
	{

	}
};

static InstallExtension<QExperimental3DExtension, Experimental3DExtensionMediator> registerExperimental3DExtension;


} // namespace udg

#endif // UDGEXPERIMENTAL3DEXTENSIONMEDIATOR_H
