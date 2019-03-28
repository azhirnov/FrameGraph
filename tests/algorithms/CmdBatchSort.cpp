// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "UnitTest_Common.h"


extern void Test_CmdBatchSort ()
{
	struct Batch
	{
		std::vector<Batch *>	dependsOn;
		const std::string		name;
		const int				queueIdx;
		bool					isReady		= false;
		bool					isDepsReady	= false;
		bool					isSubmitted	= false;

		Batch (std::string_view name, int idx) : name{name}, queueIdx{idx} {}
	};

	struct Queue
	{
		std::list<Batch*>	pending;
	};

	std::array<Queue, 3>	queues;

	auto	Flush = [&queues] (std::string_view ref)
	{
		std::vector<Batch*>	submit;
		std::string			str;
		int					max_iii = 0;
		int					max_ii	= 0;
		int					count	= 0;

		for (int iii = 0, iii_changed = 1; iii_changed and (iii < 10); ++iii)
		{
			max_iii = std::max( max_iii, iii );

			iii_changed = 0;

			for (auto& q : queues)
			{
				for (int ii = 0, ii_max = std::max(10, int(q.pending.size())), ii_changed = 1;
					 ii_changed and (ii < ii_max);
					 ++ii)
				{
					max_ii = std::max( max_ii, ii );
					ii_changed = 0;

					for (auto iter = q.pending.begin(); iter != q.pending.end();)
					{
						++count;

						auto&	curr = **iter;
						ASSERT( not curr.isSubmitted );

						if ( curr.isReady and not curr.isDepsReady )
						{
							curr.isDepsReady = true;

							for (auto& dep : curr.dependsOn)
							{
								curr.isDepsReady &= dep->isDepsReady;
							}
							
							if ( curr.isDepsReady )
							{
								iii_changed = ii_changed = 1;
								curr.isSubmitted = true;

								submit.push_back( &curr );
								iter = q.pending.erase( iter );

								continue;
							}
						}

						++iter;
					}
				}

				if ( submit.size() )
				{
					if ( str.size() ) str += " ";

					for (auto& b : submit)
					{
						if ( &b != submit.data() )	str += ", ";
						str += b->name;
					}

					submit.clear();
					str += ";";
				}
			}
		}

		FG_LOGI( "max: "s << ToString( max_ii ) << ", " << ToString( max_iii ) << ", total: " << ToString( count ) );
		TEST( str == ref );
	};


	// frame 1
	{
		auto*	b0 = new Batch{ "b0", 0 };
		auto*	b1 = new Batch{ "b1", 0 };
		auto*	b2 = new Batch{ "b2", 0 };

		b1->dependsOn.push_back( b0 );
		b2->dependsOn.push_back( b0 );

		queues[0].pending.push_back( b0 );
		queues[0].pending.push_back( b2 );
		queues[0].pending.push_back( b1 );

		b0->isReady = true;
		b1->isReady = true;
		b2->isReady = true;

		Flush( "b0, b2, b1;" );
	}

	// frame 2
	{
		auto*	b0 = new Batch{ "b0", 0 };
		auto*	b1 = new Batch{ "b1", 0 };
		auto*	b2 = new Batch{ "b2", 0 };

		b1->dependsOn.push_back( b0 );
		b2->dependsOn.push_back( b0 );

		queues[0].pending.push_back( b0 );
		queues[0].pending.push_back( b2 );
		queues[0].pending.push_back( b1 );

		b0->isReady = true;
		b1->isReady = true;

		Flush( "b0, b1;" );
		
		b2->isReady = true;

		Flush( "b2;" );
	}
	
	// frame 3
	{
		auto*	b0 = new Batch{ "b0", 0 };
		auto*	b1 = new Batch{ "b1", 0 };
		auto*	b2 = new Batch{ "b2", 0 };

		b0->dependsOn.push_back( b2 );
		b2->dependsOn.push_back( b1 );

		queues[0].pending.push_back( b0 );
		queues[0].pending.push_back( b2 );
		queues[0].pending.push_back( b1 );

		b0->isReady = true;
		b1->isReady = true;
		b2->isReady = true;

		Flush( "b1, b2, b0;" );
	}

	// frame 4
	{
		auto*	b0 = new Batch{ "b0", 1 };
		auto*	b1 = new Batch{ "b1", 2 };
		auto*	b2 = new Batch{ "b2", 0 };

		b1->dependsOn.push_back( b0 );
		b2->dependsOn.push_back( b1 );

		queues[ b0->queueIdx ].pending.push_back( b0 );
		queues[ b2->queueIdx ].pending.push_back( b2 );
		queues[ b1->queueIdx ].pending.push_back( b1 );

		b0->isReady = true;
		b1->isReady = true;
		b2->isReady = true;

		Flush( "b0; b1; b2;" );
	}
	
	// frame 5
	{
		auto*	b0 = new Batch{ "b0", 0 };
		auto*	b1 = new Batch{ "b1", 1 };
		auto*	b2 = new Batch{ "b2", 0 };
		auto*	b3 = new Batch{ "b3", 1 };

		b1->dependsOn.push_back( b0 );
		b2->dependsOn.push_back( b1 );
		b3->dependsOn.push_back( b2 );

		queues[ b0->queueIdx ].pending.push_back( b0 );
		queues[ b1->queueIdx ].pending.push_back( b1 );
		queues[ b2->queueIdx ].pending.push_back( b2 );
		queues[ b3->queueIdx ].pending.push_back( b3 );

		b0->isReady = true;
		b1->isReady = true;
		b2->isReady = true;
		b3->isReady = true;

		Flush( "b0; b1; b2; b3;" );
	}
	
	// frame 6
	{
		auto*	b0 = new Batch{ "b0", 0 };
		auto*	b1 = new Batch{ "b1", 1 };
		auto*	b2 = new Batch{ "b2", 0 };
		auto*	b3 = new Batch{ "b3", 1 };

		queues[ b0->queueIdx ].pending.push_back( b0 );
		queues[ b1->queueIdx ].pending.push_back( b1 );
		queues[ b2->queueIdx ].pending.push_back( b2 );
		queues[ b3->queueIdx ].pending.push_back( b3 );

		b0->isReady = true;
		b1->isReady = true;
		b2->isReady = true;
		b3->isReady = true;

		Flush( "b0, b2; b1, b3;" );
	}
}
