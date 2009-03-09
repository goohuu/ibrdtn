#ifndef SIMPLEBUNDLESTORAGE_H_
#define SIMPLEBUNDLESTORAGE_H_

#include "core/BundleCore.h"
#include "core/BundleStorage.h"
#include "core/EventReceiver.h"
#include "data/Bundle.h"
#include "utils/Service.h"
#include "utils/MutexLock.h"
#include "utils/Mutex.h"
#include "core/Node.h"
#include <list>
#include <map>

using namespace dtn::data;
using namespace dtn::utils;

namespace dtn
{
namespace core
{
/**
 * Implementiert einen einfachen Kontainer für Bundles
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
	SimpleBundleStorage(BundleCore &core, unsigned int size = 1024 * 1024, unsigned int bundle_maxsize = 1024, bool merge = false);

	/**
	 * Destruktor
	 */
	virtual ~SimpleBundleStorage();

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
	 * receive event from the event switch
	 */
	void raiseEvent(const Event *evt);

private:
	/**
	 * @sa BundleStorage::store(BundleSchedule schedule)
	 */
	void store(const BundleSchedule &schedule);

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
	Bundle* storeFragment(const Bundle *bundle);

	BundleCore &m_core;

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

	map<string,Node> m_neighbours;

	bool m_bundlewaiting;
};

}
}

#endif /*SIMPLEBUNDLESTORAGE_H_*/
