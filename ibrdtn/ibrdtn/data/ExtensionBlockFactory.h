/*
 * ExtensionBlockFactory.h
 *
 *  Created on: 28.05.2010
 *      Author: morgenro
 */

#ifndef EXTENSIONBLOCKFACTORY_H_
#define EXTENSIONBLOCKFACTORY_H_

#include "ibrdtn/data/Bundle.h"

namespace dtn
{
	namespace data
	{
		class ExtensionBlockFactory
		{
		public:
			virtual dtn::data::Block* create() = 0;

		protected:
			ExtensionBlockFactory(char type)
			 : _type(type)
			{
				std::map<char, ExtensionBlockFactory*> &factories = dtn::data::Bundle::getExtensionBlockFactories();
				factories[type] = this;
			}

			virtual ~ExtensionBlockFactory()
			{
				std::map<char, ExtensionBlockFactory*> &factories = dtn::data::Bundle::getExtensionBlockFactories();
				factories.erase(_type);
			}

		private:
			char _type;
		};
	}
}

#endif /* EXTENSIONBLOCKFACTORY_H_ */
