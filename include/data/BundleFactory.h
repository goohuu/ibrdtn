#ifndef BUNDLEFACTORY_H_
#define BUNDLEFACTORY_H_

#include "Bundle.h"
#include "PrimaryFlags.h"
#include "Exceptions.h"
#include "NetworkFrame.h"
#include "BlockFactory.h"
#include <vector>
#include <list>

namespace dtn
{
	namespace data
	{
		/**
		 * This is a factory class for bundle objects.
		 */
		class BundleFactory
		{
		public:
			/**
			 * Returns a static instance of the BundleFactory.
			 */
			static BundleFactory& getInstance();

			/**
			 * Register a BlockFactory for parsing extension blocks of a bundle.
			 * @param[in] f Pointer to a instance of the BlockFactory.
			 */
			void registerExtensionBlock(BlockFactory *f);

			/**
			 * Removes a BlockFactory for parsing extension blocks of a bundle.
			 * @param[in] f Pointer to a instance of the BlockFactory.
			 */
			void unregisterExtensionBlock(BlockFactory *f);

			/**
			 * Parse some given data and create a bundle object out of the data.
			 * @param[in] data A pointer to a data array of bundle data.
			 * @param[in] size The size of the data array.
			 */
			Bundle* parse(const unsigned char *data, unsigned int size) const;

			/**
			 * Create a new empty bundle object.
			 * @return A empty bundle.
			 */
			Bundle* newBundle();

			/**
			 * Makes a copy of a block.
			 * @param block The block to copy.
			 * @return A copy of the block.
			 */
			Block* copyBlock(const Block &block) const;

			/**
			 * Get the current dtn time which is the seconds since the year 2000.
			 * @return The current dtn time.
			 */
			static unsigned int getDTNTime();

			/**
			 * Get a new sequence number and count up the internal counter.
			 * @return a sequence number
			 */
			unsigned int getSequenceNumber();

			/**
			 * Split a bundle into a list of bundle fragments.
			 * @param[in] bundle The bundle to split.
			 * @param[in] maxsize The maximum size of a fragment.
			 * @returns A list of bundle fragments.
			 */
			static list<Bundle*> split(const Bundle &bundle, unsigned int maxsize);

			/**
			 * Slices a bundle into a fragment bundles with a specific size and returns it.
			 * @param[in] bundle The bundle to slice.
			 * @param[in] size The size of the fragment.
			 * @param[in,out] offset A offset which tells the method to start from. The offset
			 * is moved forward to the end of the fragment.
			 * @returns A bundle fragment.
			 */
			static Bundle* slice(const Bundle &bundle, unsigned int size, unsigned int& offset);

			/**
			 * Cut the payload of the bundle after a specific size and returns a fragment bundle.
			 * @param[in] bundle The bundle to slice.
			 * @param[in] size The payload size of the fragment.
			 * @param[in,out] offset A offset which tells the method to start from. The offset
			 * is moved forward to the end of the fragment.
			 * @returns A bundle fragment.
			 */
			static Bundle* cut(const Bundle &bundle, unsigned int size, unsigned int& offset);

			/**
			 * Cut a bundle at a given position into two fragments. The position is absolutely,
			 * so you have to use a position between the borders of the payload. Otherwise a
			 * FragmentationException is thrown.
			 * @param[in] bundle The bundle which needs to be cut at a given position.
			 * @param[in] position The position for the cut in the payload of the bundle.
			 * @return a pair of two bundles. The first bundle is the first part of the bundle.
			 */
			static pair<Bundle*, Bundle*> cutAt(const Bundle &bundle, unsigned int position);

			/**
			 * Merge a list of fragments to one bundle. If not all fragments are in the given
			 * list, the algorithm try to partly merge the bundles and put the resulting fragment
			 * into the given list. Bundle-Objects which were merged are removed out of the list
			 * and deleted by this method.
			 * @param[in,out] bundles A list of bundle objects. These bundles have to be
			 * 				  fragments. If not, a FragmentationException is thrown.
			 * @return 		  A non-fragmented bundle object.
			 */
			static Bundle* merge(list<Bundle*> &bundles);

			/**
			 * Merge two fragments to one bundle or a new fragment. If the fragments aren't
			 * bordering or overlapping a FragmentationException is thrown.
			 * @param[in] fragment1 The first fragment. The offset of this bundle has to be
			 * 			  smaller than the offset of the second fragment.
			 * @param[in] fragment2 The second fragment. The offset of this bundle has to be
			 * 			  bigger than the offset of the first fragment.
			 * @return	  A fragment if still fragments missing or a non-fragmented bundle if all
			 * 		   	  fragments available.
			 */
			static Bundle* merge(const Bundle &fragment1, const Bundle &fragment2);

		private:
			BundleFactory();
			virtual ~BundleFactory();

			BlockFactory& getExtension(unsigned char type);
			map<char, BlockFactory*>& getExtensions();

			Bundle* createBundle(bool fragmented = false);

			NetworkFrame* parsePrimaryBlock(const unsigned char *data, unsigned int size) const;

			/**
			 * compare method for sorting fragments
			 */
			static bool compareFragments(const Bundle *first, const Bundle *second);
			static Block* getBlock(const unsigned char *data, unsigned int size);

			static unsigned int SECONDS_TILL_2000;
			BlockFactory m_blockfactory;
			map<char, BlockFactory*> m_extensions;
		};
	}
}

#endif /*BUNDLEFACTORY_H_*/
