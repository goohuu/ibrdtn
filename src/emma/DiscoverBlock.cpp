#include "emma/DiscoverBlock.h"
#include "data/BundleFactory.h"
#include <cstdlib>
#include <cstring>


namespace emma
{
	DiscoverBlock::DiscoverBlock(NetworkFrame *frame) : Block(frame)
	{
		this->m_processed = true;
	}

	DiscoverBlock::DiscoverBlock(Block *block) : Block(block)
	{
		this->m_processed = true;
	}

	DiscoverBlock::~DiscoverBlock()
	{
	}

	string DiscoverBlock::getConnectionAddress()
	{
		NetworkFrame &frame = Block::getFrame();
		return frame.getString(Block::getBodyIndex() + 2);
	}

	void DiscoverBlock::setConnectionAddress(string address)
	{
		NetworkFrame &frame = Block::getFrame();
		frame.set(Block::getBodyIndex() + 1, address.length());
		frame.set(Block::getBodyIndex() + 2, address);
	}

	unsigned int DiscoverBlock::getConnectionPort()
	{
		unsigned int field = Block::getBodyIndex();
		return Block::getFrame().getSDNV( field );
	}

	void DiscoverBlock::setConnectionPort(unsigned int port)
	{
		NetworkFrame &frame = Block::getFrame();
		frame.set(Block::getBodyIndex(), port);
	}

	unsigned int DiscoverBlock::getOptionals()
	{
		return Block::getFrame().getSDNV( Block::getBodyIndex() + 3 );
	}

	void DiscoverBlock::setLatitude(double value)
	{
		NetworkFrame &frame = Block::getFrame();

		if (getOptionals() == 0)
		{
			frame.append(value);
			frame.append((double)0.0);
			frame.set( Block::getBodyIndex() + 3, 2 );
		}

		frame.set( Block::getBodyIndex() + 4, value );
	}

	double DiscoverBlock::getLatitude()
	{
		if (getOptionals() > 0)
			return Block::getFrame().getDouble( Block::getBodyIndex() + 4 );
		else
			return 0;
	}

	void DiscoverBlock::setLongitude(double value)
	{
		NetworkFrame &frame = Block::getFrame();

		if (getOptionals() == 0)
		{
			frame.append((double)0.0);
			frame.append(value);
			frame.set( Block::getBodyIndex() + 3, 2 );
		}

		frame.set( Block::getBodyIndex() + 5, value );
	}

	double DiscoverBlock::getLongitude()
	{
		if (getOptionals() > 1)
			return Block::getFrame().getDouble( Block::getBodyIndex() + 5 );
		else
			return 0;
	}
}
