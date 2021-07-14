#include<array>

/**
 *  @file pair_end_adapter_trimmer.hpp
 *  @brief provide PEAT implementation detail
 *  @author C-Salt Corp.
 */
#ifndef PAIR_END_ADAPTER_TRIMMER_HPP_
#define PAIR_END_ADAPTER_TRIMMER_HPP_
#include "pair_end_adapter_trimmer_impl.hpp"
#include "../constant_def.hpp"
/**
 * @class PairEndAdapterTrimmer
 * @brief provide a generic form of PEAT
 * @tparam none-type parameter of int / enum of ParallelTypes, indicating what policies that PEAT is going to employ
 * @tparam template template parameter of FORMAT, designed to be any one of current data formats
 * @tparam TUPLETYPE indicating data format of the got piece of data
 * @tparam TRAIT indicating the TrimTrait format that is going to apply
 */
template < 
		ParallelTypes ParaType, 
		template <typename> class FORMAT,
		typename TUPLETYPE,
		typename TRAIT
		 >
class PairEndAdapterTrimmer 
{};
 
/**
 * @class PairEndAdapterTrimmer
 * @brief specialized form of the PairEndAdapterTrimmer, with the ParallelType specialized to be ParallelTypes::NORMAL, indicating non-parallel scheme
 */
template < 
		template <typename> class FORMAT,
		typename TUPLETYPE,
		typename TRAIT
		 >
class PairEndAdapterTrimmer < ParallelTypes::NORMAL, FORMAT, TUPLETYPE, TRAIT >
	: public PairEndAdapterTrimmer_impl < FORMAT, TUPLETYPE, TRAIT >
{ 
public:
/**
 * @typedef format_type
 * @brief define a trait access for the type of FORMAT<TUPLETYPE>
 */
	typedef FORMAT<TUPLETYPE> format_type;
/**
 * @typedef tuple_type
 * @brief define a trait access for the type of TUPLETYPE
 */
	typedef TUPLETYPE tuple_type;
/**
 * @typedef peat_trait_type
 * @brief define a trait access for the type of TRAIT
 */
	typedef TRAIT peat_trait_type;
/**
 * @typedef parameter_trait_type
 * @brief define a trait access for the type of ParameterTrait<TRAIT>
 */
	typedef ParameterTrait <TRAIT> parameter_trait_type;
/**
 * @brief define a data structure keeping track of the trimming position obtained in Trim funciton 
 */
	std::shared_ptr < std::map < int, std::vector<size_t> > > trim_position;
/**
 * @brief constructor
 */
	PairEndAdapterTrimmer (//std::map<int, std::vector< FORMAT<TUPLETYPE> > >* content_ptr, 
						 	 ParameterTrait <TRAIT> i_parameter_trait )
		: PairEndAdapterTrimmer_impl < FORMAT, TUPLETYPE, TRAIT >
				( //content_ptr, 
				i_parameter_trait )
		, trim_position ( std::make_shared<std::map < int, std::vector<size_t> > > () ) 
	{}

/**
 * @brief main interface for conducting trimming operation.  
 */
	void Trim (std::map < int, std::vector< FORMAT<TUPLETYPE> > >* result2 )
	{
//		return ;
		this->check_read_length (result2);
		auto job_count = std::min ( this->parameter_trait.num, std::min ( (*result2)[0].size(), (*result2)[1].size() ) );
		if (job_count >= this->parameter_trait.pool_size)
		{
			size_t dif = job_count/this->parameter_trait.pool_size;
			for (size_t i=0; i!=this->parameter_trait.pool_size; ++i)
				this->TrimImpl(result2, (*trim_position)[i], i*dif, dif ); 
		}
		else
		{
			std::cerr<<"job_count less than pool_size "<<job_count<<'\t'<<this->parameter_trait.pool_size<<std::endl;
			this->TrimImpl (result2, (*trim_position)[0], 0, job_count);
		}
	}
};

/**
 * @class PairEndAdapterTrimmer
 * @brief specialized form of the PairEndAdapterTrimmer, with the ParallelType specialized to be ParallelTypes::NORMAL, indicating multi-threading parallel scheme
 */
template < 
		template <typename> class FORMAT,
		typename TUPLETYPE,
		typename TRAIT
		 >
class PairEndAdapterTrimmer < ParallelTypes::M_T, FORMAT, TUPLETYPE, TRAIT >
	: public PairEndAdapterTrimmer_impl < FORMAT, TUPLETYPE, TRAIT >
{ 
protected:
/**
 * @memberof PairEndAdapterTrimmer
 * @brief define a normal pointer pointing to a ThreadPool object
 */
	ThreadPool* ptr_to_GlobalPool_peat_;

public:
/**
 * @typedef format_type
 * @brief define a trait access for the type of FORMAT<TUPLETYPE>
 */
	typedef FORMAT<TUPLETYPE> format_type;
/**
 * @typedef tuple_type
 * @brief define a trait access for the type of TUPLETYPE
 */
	typedef TUPLETYPE tuple_type;
/**
 * @typedef peat_trait_type
 * @brief define a trait access for the type of TRAIT
 */
	typedef TRAIT peat_trait_type;
/**
 * @typedef parameter_trait_type
 * @brief define a trait access for the type of ParameterTrait<TRAIT>
 */
	typedef ParameterTrait <TRAIT> parameter_trait_type;
/**
 * @memberof PairEndAdapterTrimmer
 * @brief define a data structure keeping track of the trimming position obtained in Trim funciton 
 */
	std::shared_ptr < std::map < int, std::vector<size_t> > > trim_position;
/**
 * @brief constructor
 */
	PairEndAdapterTrimmer ( //std::map<int, std::vector< FORMAT<TUPLETYPE> > >* content_ptr, 
							  ParameterTrait <TRAIT> i_parameter_trait,
							  ThreadPool* tp = &GlobalPool )
		: PairEndAdapterTrimmer_impl < FORMAT, TUPLETYPE, TRAIT >
				( //content_ptr, 
				i_parameter_trait )
		, ptr_to_GlobalPool_peat_ (tp)
		, trim_position ( std::make_shared<std::map < int, std::vector<size_t> > > () ) 
	{
		ptr_to_GlobalPool_peat_->ChangePoolSize(i_parameter_trait.pool_size);
	}

	void QTrim (std::map < int, std::vector< FORMAT<TUPLETYPE> > >* result2)
	{
		if ( (*result2)[0].size()==0 )
			return;
		this->check_read_length (result2);
		auto job_count = std::min ( this->parameter_trait.num, std::min ( (*result2)[0].size(), (*result2)[1].size() ) );
		std::vector<size_t> index_vec;
		if (job_count >= this->parameter_trait.pool_size)
		{
			size_t dif = job_count/this->parameter_trait.pool_size;
			for (size_t i=0; i!=this->parameter_trait.pool_size; ++i)
			{
				index_vec.push_back ( 
					ptr_to_GlobalPool_peat_ -> JobPost ( 
						boost::bind	( 
							&PairEndAdapterTrimmer_impl <FORMAT, TUPLETYPE, TRAIT >::QTrimImpl, 
							this, result2, i*dif, dif
						) 
					) 
				);
			}

			for (auto& i : index_vec)
			{
				ptr_to_GlobalPool_peat_->FlushOne (i);
			}
		}   
		else
		{
			this->QTrimImpl (result2, 0, job_count);
		}
	}

/**
 * @brief main interface for conducting trimming operation.  
 */

	void Trim (std::map < int, std::vector< FORMAT<TUPLETYPE> > >* result2, int nthreads)
	{

		if ( (*result2)[0].size()==0 )
			return;

		this->check_read_length (result2);
		auto job_count = std::min ( this->parameter_trait.num, std::min ( (*result2)[0].size(), (*result2)[1].size() ) );
		std::vector<size_t> index_vec;
		if (job_count >= this->parameter_trait.pool_size)
		{
			size_t dif = job_count/this->parameter_trait.pool_size;
			for (size_t i=0; i!=this->parameter_trait.pool_size; ++i)
			{
				index_vec.push_back ( 
					ptr_to_GlobalPool_peat_ -> JobPost ( 
						boost::bind	( 
							&PairEndAdapterTrimmer_impl <FORMAT, TUPLETYPE, TRAIT >::TrimImpl, 
							this, result2, boost::ref ( (*trim_position)[i] ), i*dif, dif
						) 
					) 
				);
			}

			for (auto& i : index_vec)
				ptr_to_GlobalPool_peat_->FlushOne (i);
		}   
		else
			this->TrimImpl (result2, (*trim_position)[0], 0, job_count);

	}
	void Verbose ( bool flag, uint32_t count_reads, int& flag_type )
	{
		this->VerboseImpl ( flag, count_reads, flag_type );
	}

	void Sum ( std::map < int, std::vector< FORMAT<TUPLETYPE> > >* result, uint32_t& sum_read_lengths, uint32_t& sum_read_counts, int map_index=0 )
	{
		this-> SumImpl( (*result)[ map_index ], sum_read_lengths, sum_read_counts );
		this-> SumImpl( (*result)[ map_index + 1 ], sum_read_lengths, sum_read_counts );
	}	
	
	void Summary ( uint32_t sum_length, uint32_t sum_reads, int value, std::ostream* out_report )
	{                                                                                                              
        double average_length ( double(sum_length)/double(sum_reads) );
		if ( value == 0 )
		{	
			(*out_report) << "PEAT report\nMode:\tpaired-end";
			(*out_report) << "\nTotal number of reads:\t" << sum_reads;
			(*out_report) << "\nAverage length of reads in raw data:\t" << average_length;
		}
		else if ( value == 1 )
		{
			(*out_report) << "\nAverage length of reads after quality trimming:\t" << average_length;
		}
	}                                                                                                              
	
	void Summary ( uint32_t sum_length, uint32_t sum_reads, std::ostream* out_report, std::map<std::string, int>& adapter_context_set )
	{
        double average_length ( double(sum_length)/double(sum_reads) );
		(*out_report) << "\nAverage length of reads after adapter trimming:\t" << average_length << "\n";

		if( !adapter_context_set.empty() )
		{
			std::multimap<int, std::string> adapter_context_rank;
			(*out_report)<< "The top numbers of adpater contexts:\n";	
			(*out_report)<< "Context\tNumber\n";	

			int count(0);
			for ( auto& element: adapter_context_set )
				adapter_context_rank.insert( {element.second, element.first} );
			for ( auto riter = adapter_context_rank.rbegin(); riter != adapter_context_rank.rend(); ++riter)
			{
				if ( count !=10 ) 
					(*out_report) << riter->second << "\t" << riter->first << "\n";
				else
					break;
				++count;
			}
		}
	}

};

#endif
