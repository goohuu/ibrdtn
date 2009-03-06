#ifndef BUNDLESTORAGE_H_
#define BUNDLESTORAGE_H_

#include "core/BundleSchedule.h"
#include "data/Bundle.h"
#include <vector>

using namespace dtn::data;

namespace dtn
{
	namespace core
	{
		/**
		 * Die BundleStorage verwahrt Bundles wenn diese nicht direkt weitergeleitet werden
		 * können oder eine Kopie des Bundles aufgehoben wird.
		 */
		class BundleStorage
		{
			public:
			/**
			 * Konstruktor
			 */
			BundleStorage() {};

			/**
			 * Desktruktor
			 */
			virtual ~BundleStorage() {};

			/**
			 * Speichert einen Schedule
			 * @param schedule Den zu speichernden Schedule
			 */
			virtual void store(BundleSchedule schedule) = 0;

			/**
			 * Speichert ein Fragment eines Bundles. Falls dies das fehlende letzte
			 * Fragment war, wird das Bundle zusammengesetzt und zurück gegeben,
			 * ansonsten NULL.
			 */
			virtual Bundle* storeFragment(Bundle *bundle) = 0;

			/**
			 * Leert den Speicher / Löscht alle Bundles
			 */
			virtual void clear() = 0;

			/**
			 * Bestimmt ob der Speicher keine Bundles enthält
			 * @return True, wenn keine Bundles im Speicher sind, sonst False
			 */
			virtual bool isEmpty() = 0;

			/**
			 * Returns the count of bundles in the storage
			 * @return the count of bundles in the storage
			 */
			virtual unsigned int getCount() = 0;

//			/**
//			 * Gibt ein Schedule zurück welches zu dem angegebenen Zeitpunkt gesendet werden soll
//			 * und entfernt es aus dem Storage
//			 */
//			virtual BundleSchedule getSchedule(unsigned int dtntime) = 0;
//
//			/**
//			 * Gibt ein Bundle der für einen bestimmten Knoten bestimmt ist zurück
//			 * und entfernt das entsprechende Schedule aus der Storage
//			 */
//			virtual BundleSchedule getSchedule(string destination) = 0;
		};
	}
}

#endif /*BUNDLESTORAGE_H_*/
