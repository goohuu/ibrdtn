#ifndef SIMPLEBUNDLESTORAGE_H_
#define SIMPLEBUNDLESTORAGE_H_

#include "core/BundleStorage.h"
#include "core/EventReceiver.h"
#include "data/Bundle.h"
#include "utils/Service.h"
#include "utils/MutexLock.h"
#include "utils/Mutex.h"
#include <list>
#include <vector>

using namespace dtn::data;
using namespace dtn::utils;

namespace dtn
{
namespace core
{
/**
 * Implementiert einen einfachen Kontainer für Bundles
 *
 * TODO: Store copies of bundles in the storage NOT pointer!
 */
class SimpleBundleStorage : public Service, public BundleStorage, public EventReceiver
{
public:
	/**
	 * Constructor
	 * @param[in] size Size of the memory storage in kBytes. (e.g. 1024*1024 = 1MB)
	 * @param[in] bundle_maxsize Maximum size of one bundle in bytes.
	 * @param[in] merge Do database cleanup and merging.
	 */
	SimpleBundleStorage(unsigned int size = 1024 * 1024, unsigned int bundle_maxsize = 1024, bool merge = false);

	/**
	 * Destruktor
	 */
	virtual ~SimpleBundleStorage();

	/**
	 * @sa BundleStorage::store(BundleSchedule schedule)
	 */
	void store(BundleSchedule schedule);

	/**
	 * @sa BundleStorage::clear()
	 */
	void clear();

	/**
	 * @sa BundleStorage::isEmpty()
	 */
	bool isEmpty();

	/**
	 * @sa BundleStorage::getCount()
	 */
	unsigned int getCount();

	/**
	 * @sa Service::tick()
	 */
	virtual void tick();

	/**
	 * @sa getSchedule(unsigned int dtntime)
	 */
	BundleSchedule getSchedule(unsigned int dtntime);

	/**
	 * @sa getSchedule(string destination)
	 */
	BundleSchedule getSchedule(string destination);

	/**
	 * @sa storeFragment();
	 */
	Bundle* storeFragment(Bundle *bundle);

	/**
	 * receive event from the event switch
	 */
	void raiseEvent(const Event *evt);

private:
	list<BundleSchedule> m_schedules;
	list<list<Bundle*> > m_fragments;

	list<list<BundleSchedule>::iterator> searchEquals(unsigned int maxsize, list<BundleSchedule>::iterator start, list<BundleSchedule>::iterator end);

	/**
	 * Löscht veraltete Bundles bzw. versucht Bundles zusammenzufassen,
	 * so dass diese weniger Speicherplatz verbrauchen
	 */
	void deleteDeprecated();

	// Der nächste Zeitpunkt zu dem ein Bündel abläuft
	// Frühestens zu diesem Zeitpunkt muss ein deleteDeprecated() ausgeführt werden
	unsigned int m_nextdeprecation;

	time_t m_last_compress;

	unsigned int m_size;
	unsigned int m_bundle_maxsize;

	unsigned int m_currentsize;

	bool m_nocleanup;

	Mutex m_readlock;
	Mutex m_fragmentslock;

	bool m_merge;
};

}
}

#endif /*SIMPLEBUNDLESTORAGE_H_*/
