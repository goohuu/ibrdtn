#ifndef SIMPLEBUNDLESTORAGE_H_
#define SIMPLEBUNDLESTORAGE_H_

#include "Component.h"
#include "core/BundleCore.h"
#include "core/BundleStorage.h"
#include "core/Node.h"
#include "core/EventReceiver.h"
#include "routing/BundleList.h"

#include <ibrcommon/thread/Conditional.h>
#include <ibrcommon/thread/AtomicCounter.h>
#include <ibrcommon/thread/Mutex.h>

#include <ibrcommon/data/File.h>
#include <ibrdtn/data/Bundle.h>

#include <set>

using namespace dtn::data;

namespace dtn
{
	namespace core
	{
		/**
		 * This storage holds all bundles and fragments in the system memory.
		 */
		class SimpleBundleStorage : public BundleStorage, public EventReceiver, public dtn::daemon::IntegratedComponent
		{
		public:
			/**
			 * Constructor
			 */
			SimpleBundleStorage();

			/**
			 * Constructor
			 */
			SimpleBundleStorage(const ibrcommon::File &workdir);

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
			virtual dtn::data::Bundle get(const dtn::data::EID &eid);


			const std::list<dtn::data::BundleID> getList() const;

			/**
			 * This method deletes a specific bundle in the storage.
			 * No reports will be generated here.
			 * @param id The ID of the bundle to remove.
			 */
			void remove(const dtn::data::BundleID &id);

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
			 * This method is used to receive events.
			 * @param evt
			 */
			void raiseEvent(const Event *evt);

		protected:
			virtual void componentUp();
			virtual void componentDown();

		private:
			enum RunMode
			{
				MODE_NONPERSISTENT = 0,
				MODE_PERSISTENT = 1
			};

			class BundleContainer : public dtn::data::BundleID
			{
			public:
				BundleContainer(const dtn::data::Bundle &b);
				BundleContainer(const ibrcommon::File &file);
				BundleContainer(const dtn::data::Bundle &b, const ibrcommon::File &workdir);
				~BundleContainer();

				bool operator!=(const BundleContainer& other) const;
				bool operator==(const BundleContainer& other) const;
				bool operator<(const BundleContainer& other) const;
				bool operator>(const BundleContainer& other) const;

				dtn::data::Bundle& operator*();
				const dtn::data::Bundle& operator*() const;

				BundleContainer& operator= (const BundleContainer &right);
				BundleContainer& operator= (BundleContainer &right);
				BundleContainer(const BundleContainer& right);

				void remove();

			protected:
				class Holder
				{
				public:
					Holder( const dtn::data::Bundle &b );
					Holder( const ibrcommon::File &file );
					Holder( const dtn::data::Bundle &b, const ibrcommon::File &workdir );
					~Holder();

					dtn::data::Bundle _bundle;
					ibrcommon::File _container;
					RunMode _mode;
					unsigned _count;

					bool deletion;
				};

			private:
				Holder *_holder;
			};

			class BundleStore : private dtn::routing::BundleList
			{
			public:
				BundleStore();
				BundleStore(ibrcommon::File workdir);
				~BundleStore();

				void load(const ibrcommon::File &file);
				void store(const dtn::data::Bundle &bundle);
				void remove(const dtn::data::BundleID &id);
				void clear();

				dtn::data::Bundle get(const dtn::data::EID &eid);
				dtn::data::Bundle get(const dtn::data::BundleID &id);

				const std::list<dtn::data::BundleID> getList() const;

				void expire(const size_t timestamp);

				std::set<BundleContainer> bundles;
				ibrcommon::Mutex bundleslock;

			protected:
				virtual void eventBundleExpired(const ExpiringBundle &b);

			private:
				ibrcommon::File _workdir;
				RunMode _mode;
			};

			BundleStore _store;
			bool _running;
		};
	}
}

#endif /*SIMPLEBUNDLESTORAGE_H_*/
