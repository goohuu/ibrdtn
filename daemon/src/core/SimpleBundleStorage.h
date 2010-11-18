#ifndef SIMPLEBUNDLESTORAGE_H_
#define SIMPLEBUNDLESTORAGE_H_

#include "Component.h"
#include "core/BundleCore.h"
#include "core/BundleStorage.h"
#include "core/Node.h"
#include "core/EventReceiver.h"

#include <ibrcommon/thread/Conditional.h>
#include <ibrcommon/thread/AtomicCounter.h>
#include <ibrcommon/thread/Mutex.h>

#include <ibrcommon/data/File.h>
#include <ibrdtn/data/Bundle.h>
#include <ibrdtn/data/BundleList.h>
#include <ibrcommon/thread/Queue.h>

#include <set>

using namespace dtn::data;

namespace dtn
{
	namespace core
	{
		/**
		 * This storage holds all bundles and fragments in the system memory.
		 */
		class SimpleBundleStorage : public BundleStorage, public EventReceiver, public dtn::daemon::IndependentComponent, private dtn::data::BundleList
		{
			class Task
			{
			public:
				virtual ~Task() {};
				virtual void run(SimpleBundleStorage &storage) = 0;
			};

		public:
			/**
			 * Constructor
			 */
			SimpleBundleStorage(size_t maxsize = 0);

			/**
			 * Constructor
			 */
			SimpleBundleStorage(const ibrcommon::File &workdir, size_t maxsize = 0);

			/**
			 * Destructor
			 */
			virtual ~SimpleBundleStorage();

			/**
			 * Stores a bundle in the storage.
			 * @param bundle The bundle to store.
			 */
			virtual void store(const dtn::data::Bundle &bundle);

			/**
			 * This method returns a specific bundle which is identified by
			 * its id.
			 * @param id The ID of the bundle to return.
			 * @return A bundle object of the
			 */
			virtual dtn::data::Bundle get(const dtn::data::BundleID &id);

			/**
			 * Query for a bundle with a specific destination. Set exact to true, if the application
			 * part should be compared too.
			 * @param eid
			 * @param exact
			 * @return
			 */
			virtual const dtn::data::MetaBundle getByDestination(const dtn::data::EID &eid, bool exact = false);

			/**
			 * Returns a bundle ID which is not in the bloomfilter, but in the storage
			 * @param filter
			 * @return
			 */
			virtual const dtn::data::MetaBundle getByFilter(const ibrcommon::BloomFilter &filter);

			/**
			 * Returns all bundles in the storage as a list of IDs
			 * @return
			 */
			const std::list<dtn::data::BundleID> getList();

			/**
			 * This method deletes a specific bundle in the storage.
			 * No reports will be generated here.
			 * @param id The ID of the bundle to remove.
			 */
			void remove(const dtn::data::BundleID &id);

			/**
			 * Remove one bundles which match this filter
			 * @param filter
			 * @return The bundle meta data of the removed bundle.
			 */
			dtn::data::MetaBundle remove(const ibrcommon::BloomFilter &filter);

			/**
			 * @sa BundleStorage::clear()
			 */
			void clear();

			/**
			 * @sa BundleStorage::empty()
			 */
			bool empty();

			/**
			 * @sa BundleStorage::count()
			 */
			unsigned int count();

			/**
			 * Get the current size
			 */
			size_t size() const;

			/**
			 * @sa BundleStorage::releaseCustody();
			 */
			void releaseCustody(dtn::data::BundleID &bundle);

			/**
			 * This method is used to receive events.
			 * @param evt
			 */
			void raiseEvent(const Event *evt);

			/**
			 * @see Component::getName()
			 */
			virtual const std::string getName() const;

			/**
			 * Invoke the expiration process
			 * @param timestamp
			 */
			void invokeExpiration(const size_t timestamp);

		protected:
			virtual void componentUp();
			virtual void componentRun();
			virtual void componentDown();

			bool __cancellation();

			virtual void eventBundleExpired(const ExpiringBundle &b);

		private:
			enum RunMode
			{
				MODE_NONPERSISTENT = 0,
				MODE_PERSISTENT = 1
			};

			class BundleContainer : public dtn::data::MetaBundle
			{
			public:
				BundleContainer(const dtn::data::Bundle &b);
				BundleContainer(const ibrcommon::File &file);
				BundleContainer(const dtn::data::Bundle &b, const ibrcommon::File &workdir, const size_t size);
				~BundleContainer();

				bool operator<(const BundleContainer& other) const;

				size_t size() const;

				dtn::data::Bundle get() const;

				BundleContainer& operator= (const BundleContainer &right);
				BundleContainer(const BundleContainer& right);

				void invokeStore();

				void remove();

			protected:
				class Holder
				{
				public:
					Holder( const dtn::data::Bundle &b );
					Holder( const ibrcommon::File &file );
					Holder( const dtn::data::Bundle &b, const ibrcommon::File &workdir, const size_t size );
					~Holder();

					size_t size() const;

					void invokeStore();

					dtn::data::Bundle getBundle();

					void remove();

					unsigned _count;

					ibrcommon::Mutex lock;

				private:
					enum STATE
					{
						HOLDER_MEMORY = 0,
						HOLDER_PENDING = 1,
						HOLDER_STORED = 2,
						HOLDER_DELETED = 3
					};

					ibrcommon::Mutex _state_lock;
					STATE _state;

					dtn::data::Bundle _bundle;
					ibrcommon::File _container;

					size_t _size;
				};

			private:
				void down();
				Holder *_holder;
			};

			class TaskStoreBundle : public Task
			{
			public:
				TaskStoreBundle(const SimpleBundleStorage::BundleContainer&, size_t retry = 0);
				~TaskStoreBundle();
				virtual void run(SimpleBundleStorage &storage);

			private:
				SimpleBundleStorage::BundleContainer _container;
				size_t _retry;
			};

			class TaskRemoveBundle : public Task
			{
			public:
				TaskRemoveBundle(const SimpleBundleStorage::BundleContainer&);
				~TaskRemoveBundle();
				virtual void run(SimpleBundleStorage &storage);

			private:
				SimpleBundleStorage::BundleContainer _container;
			};

			class TaskExpireBundles : public Task
			{
			public:
				TaskExpireBundles(const size_t &timestamp);
				~TaskExpireBundles();
				virtual void run(SimpleBundleStorage &storage);

			private:
				const size_t _timestamp;
			};

			void load(const ibrcommon::File &file);

			bool _running;

			ibrcommon::Queue<Task*> _tasks;

			std::set<BundleContainer> _bundles;
			ibrcommon::Mutex _bundleslock;

			ibrcommon::File _workdir;
			RunMode _mode;

			size_t _maxsize;
			size_t _currentsize;
		};
	}
}

#endif /*SIMPLEBUNDLESTORAGE_H_*/
