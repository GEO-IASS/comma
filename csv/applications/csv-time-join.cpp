// This file is part of comma, a generic and flexible library
// Copyright (c) 2011 The University of Sydney
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of the University of Sydney nor the
//    names of its contributors may be used to endorse or promote products
//    derived from this software without specific prior written permission.
//
// NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
// GRANTED BY THIS LICENSE.  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
// HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


/// @author vsevolod vlaskine

#include <deque>
#include <iostream>
#include <string>
#include <boost/scoped_ptr.hpp>
#include "../../application/command_line_options.h"
#include "../../application/contact_info.h"
#include "../../application/signal_flag.h"
#include "../../base/types.h"
#include "../../csv/stream.h"
#include "../../io/stream.h"
#include "../../csv/traits.h"
#include "../../io/select.h"
#include "../../name_value/parser.h"
#include "../../string/string.h"
#include "../../visiting/traits.h"

static void usage()
{
    std::cerr << std::endl;
    std::cerr << "a quick utility on the popular demand:" << std::endl;
    std::cerr << "join timestamped data from stdin with corresponding" << std::endl;
    std::cerr << "timestamped data from the second input" << std::endl;
    std::cerr << std::endl;
    std::cerr << "timestamps are expected to be fully ordered" << std::endl;
    std::cerr << std::endl;
    std::cerr << "note: on windows only files are supported as bounding data" << std::endl;
    std::cerr << std::endl;
    std::cerr << "usage: cat a.csv | csv-time-join <how> [<options>] bounding.csv [-] > joined.csv" << std::endl;
    std::cerr << std::endl;
    std::cerr << "<how>" << std::endl;
    std::cerr << "    --by-lower: join by lower timestamp" << std::endl;
    std::cerr << "    --by-upper: join by upper timestamp" << std::endl;
    std::cerr << "                default: --by-lower" << std::endl;
    std::cerr << "    --nearest: join by nearest timestamp" << std::endl;
    std::cerr << "               if 'block' given in --fields, output the whole block" << std::endl;
    std::cerr << "    --realtime: output immediately with current latest bounding timestamp" << std::endl;
    std::cerr << "                i.e wait for upper bound in bounding stream" << std::endl;
    std::cerr << "                attention: in this mode, it is possible that the latest bounding timestamp" << std::endl;
    std::cerr << "                           may be less or greater than the timestamp on stdin" << std::endl;
    std::cerr << "                           (i.e. no timestamp comparisons are made before outputting a record)" << std::endl;
    //std::cerr << "    --nearest-only: output only the input points nearest to timestamp" << std::endl;
    //std::cerr << "               if 'block' given in --fields, output the whole block" << std::endl;
    std::cerr << std::endl;
    std::cerr << "limitation" << std::endl;
    std::cerr << "    data with timestamp before the first and after the last bounding timestamps will be discarded at the moment; this is unwanted behaviour when using --nearest and --bound=n" << std::endl;
    std::cerr << "    workaround" << std::endl;
    std::cerr << "        use csv-time-delay to shift data timestamp and/or possibly add a bounding timestamp record with distant time in the past or future" << std::endl;
    std::cerr << std::endl;
    std::cerr << "<input/output options>" << std::endl;
    std::cerr << "    -: if csv-time-join - b.csv, concatenate output as: <stdin><b.csv>" << std::endl;
    std::cerr << "       if csv-time-join b.csv -, concatenate output as: <b.csv><stdin>" << std::endl;
    std::cerr << "       default: csv-time-join - b.csv" << std::endl;
    std::cerr << "    --binary,-b <format>: binary format" << std::endl;
    std::cerr << "    --delimiter,-d <delimiter>: ascii only; default ','" << std::endl;
    std::cerr << "    --fields,-f <fields>: input fields; default: t" << std::endl;
    std::cerr << "    --bound=<seconds>: if present, output only points inside of bound in second as double" << std::endl;
    std::cerr << "    --do-not-append,--select: if present, do not append any field from the second input" << std::endl;
    std::cerr << "    --no-discard: do not discard input points" << std::endl;
    std::cerr << "                         default: discard input points that cannot be" << std::endl;
    std::cerr << "                         consistently timestamped, especially head or tail" << std::endl;
    std::cerr << "    --discard-bounding: discard bounding data" << std::endl;
    std::cerr << "                         default: no discard" << std::endl;
    std::cerr << "    --buffer: bounding data buffer size" << std::endl;
    std::cerr << "                         default: infinite" << std::endl;
    std::cerr << "    --timestamp-only,--time-only: join only timestamp from the second input" << std::endl;
    std::cerr << "                                  otherwise join the whole line" << std::endl;
    std::cerr << std::endl;
    std::cerr << "examples" << std::endl;
    std::cerr << "    first field on stdin is timestamp, the first field of filter is timestamp" << std::endl;
    std::cerr << "        - default:" << std::endl;
    std::cerr << "            cat a.csv | csv-time-join b.csv" << std::endl;
    std::cerr << "        - explicit:" << std::endl;
    std::cerr << "            cat a.csv | csv-time-join --fields=t \"b.csv;fields=t\"" << std::endl;
    std::cerr << std::endl;
    std::cerr << "    3rd field on stdin is timestamp, the 2nd field of filter is timestamp" << std::endl;
    std::cerr << "        cat a.csv | csv-time-join --fields=,,t \"b.csv;fields=,t\"" << std::endl;
    std::cerr << std::endl;
    std::cerr << comma::contact_info << std::endl;
    std::cerr << std::endl;
    exit( -1 );
}

struct Point
{
    boost::optional<boost::posix_time::ptime> timestamp;
    Point() {}
    Point( const boost::posix_time::ptime& timestamp ) : timestamp( timestamp ) {}
};

namespace comma { namespace visiting {

template <> struct traits< Point >
{
    template < typename K, typename V > static void visit( const K&, const Point& p, V& v )
    { 
        v.apply( "t", p.timestamp );
    }
    
    template < typename K, typename V > static void visit( const K&, Point& p, V& v )
    {
        v.apply( "t", p.timestamp );
    }
};
    
} } // namespace comma { namespace visiting {


bool by_upper;
bool nearest;
bool by_lower;
bool timestamp_only;
bool select_only;

comma::csv::options stdin_csv;
comma::csv::options csv;
boost::optional< boost::posix_time::time_duration > bound;
typedef std::pair< boost::posix_time::ptime, std::string > timestring_t;

boost::posix_time::ptime get_time (const Point p)
{
    return p.timestamp ? *p.timestamp : boost::posix_time::microsec_clock::universal_time();
}

void output(const timestring_t & input, const timestring_t& joined)
{   
    if (bound && (input.first - joined.first > bound || joined.first - input.first > bound) ) { return; }
    if (stdin_csv.binary())
    {
        std::cout.write(&input.second[0], stdin_csv.format().size());
        if (select_only) { return; }
        
        if (timestamp_only)
        {
            static const unsigned int time_size = comma::csv::format::traits< boost::posix_time::ptime, comma::csv::format::time >::size;
            static char timestamp[ time_size ];
            comma::csv::format::traits< boost::posix_time::ptime, comma::csv::format::time >::to_bin( joined.first, timestamp );
            std::cout.write( (char*)&timestamp, time_size );
        }
        else
        {
            std::cout.write(&joined.second[0], csv.format().size());
        }
    }
    else
    {
        std::cout << input.second;
        if ( select_only ) { return; } 
        
        if (timestamp_only)
        {
            std::cout << stdin_csv.delimiter << boost::posix_time::to_iso_string(joined.first);
        }
        else 
        {
            std::cout << stdin_csv.delimiter << joined.second << std::endl;
        }
    }
    std::cout.flush();
}

int main( int ac, char** av )
{
    try
    {
        comma::signal_flag is_shutdown(comma::signal_flag::hard);
        comma::command_line_options options( ac, av );
        if( options.exists( "--help" ) || options.exists( "-h" ) || ac == 1 ) { usage(); }
        options.assert_mutually_exclusive( "--by-lower,--by-upper,--nearest" );
        by_upper = options.exists( "--by-upper" );
        nearest = options.exists( "--nearest" );
        by_lower = ( options.exists( "--by-lower" ) || !by_upper ) && !nearest;
        timestamp_only = options.exists( "--timestamp-only,--time-only" );
        select_only = options.exists( "--do-not-append,--select" );
        if( select_only && timestamp_only ) { std::cerr << "csv-time-join: --timetamp-only specified with --select, ignoring --timestamp-only" << std::endl; }
        bool discard_bounding = options.exists( "--discard-bounding" );
        boost::optional< unsigned int > buffer_size;
        if( options.exists( "--buffer" ) ) { buffer_size = options.value< unsigned int >( "--buffer" ); }
        bool discard = !options.exists( "--no-discard" );
        if( options.exists( "--bound" ) ) { bound = boost::posix_time::microseconds( options.value< double >( "--bound" ) * 1000000 ); }
        stdin_csv = comma::csv::options( options, "t" );
        comma::csv::input_stream< Point > stdin_stream( std::cin, stdin_csv );
        #ifdef WIN32
        if( stdin_csv.binary() ) { _setmode( _fileno( stdout ), _O_BINARY ); }
        #endif // #ifdef WIN32
        std::vector< std::string > unnamed = options.unnamed( "--by-lower,--by-upper,--nearest,--select,--do-not-append,--timestamp-only,--time-only,--no-discard,--discard-bounding,--realtime", "--binary,-b,--delimiter,-d,--fields,-f,--bound,--buffer,--verbose,-v" );
        std::string properties;
        bool bounded_first = true;
        switch( unnamed.size() )
        {
            case 0:
                std::cerr << "csv-time-join: please specify bounding source" << std::endl;
                return 1;
            case 1:
                properties = unnamed[0];
                break;
            case 2:
                if( unnamed[0] == "-" ) { properties = unnamed[1]; }
                else if( unnamed[1] == "-" ) { properties = unnamed[0]; bounded_first = false; }
                else { std::cerr << "csv-time-join: expected either '- <bounding>' or '<bounding> -'; got : " << comma::join( unnamed, ' ' ) << std::endl; return 1; }
                break;
            default:
                std::cerr << "csv-time-join: expected either '- <bounding>' or '<bounding> -'; got : " << comma::join( unnamed, ' ' ) << std::endl;
                return 1;
        }
        comma::name_value::parser parser( "filename" );
        csv = parser.get< comma::csv::options >( properties );
        if( csv.fields.empty() ) { csv.fields = "t"; }
        comma::io::istream is( comma::split( properties, ';' )[0], csv.binary() ? comma::io::mode::binary : comma::io::mode::ascii );
        comma::csv::input_stream< Point > istream( *is, csv );
        std::deque<timestring_t> bounding_queue;
        #ifndef WIN32
        comma::io::select select;
        comma::io::select istream_select;
        select.read().add(0);
        select.read().add(is.fd());
        istream_select.read().add(is.fd());
        #endif // #ifndef WIN32
        const Point* p = NULL;
        bool next=true;

        bool bounding_data_available;
        if (options.exists("--realtime"))
        {
            #ifndef WIN32
            bool end_of_input = false;
            bool end_of_bounds = false;
            
            boost::optional<timestring_t> joined_line;
            
            while (!is_shutdown && !end_of_input)
            {
                if ( !istream.ready() && !stdin_stream.ready() )
                {
                    select.wait(boost::posix_time::milliseconds(1));
                }
                
                if ( !is_shutdown && !end_of_input && ( stdin_stream.ready() || ( select.check() && select.read().ready( comma::io::stdin_fd ) ) ) )
                {
                    p = stdin_stream.read();
                    if (!p)
                    {
                        comma::verbose << "end of input stream" << std::endl;
                        end_of_input = true;
                    }
                    timestring_t input_line = std::make_pair(get_time(*p), stdin_stream.last());
                    if (joined_line) { output(input_line, *joined_line); }
                }
                
                if ( !is_shutdown && !end_of_bounds && ( istream.ready() || ( select.check() && select.read().ready( is.fd() ) ) ) )
                {
                    p = istream.read();
                    if (!p)
                    {
                        comma::verbose << "end of bounding stream" << std::endl;
                        end_of_bounds = true;
                    }
                    joined_line = std::make_pair(get_time(*p), istream.last());
                }
            }
            if (is_shutdown) { comma::verbose << "got a signal" << std::endl; return 0; }
            #else
            COMMA_THROW(comma::exception, "--realtime mode not supported in WIN32");
            #endif
        }
        else
        {
          while( ( stdin_stream.ready() || ( std::cin.good() && !std::cin.eof() ) ) )
          {
              bounding_data_available =  istream.ready() || ( is->good() && !is->eof());
              #ifdef WIN32
              bool istream_ready = true;
              bool stdin_stream_ready = true;
              #else // #ifdef WIN32
              //check so we do not block
              bool istream_ready = istream.ready();
              bool stdin_stream_ready = stdin_stream.ready();

              if(next)
              {
                  if(!istream_ready || !stdin_stream_ready)
                  {
                      if(!istream_ready && !stdin_stream_ready)
                      {
                          select.wait(boost::posix_time::milliseconds(10));
                      }
                      else
                      {
                          select.check();
                      }
                      if(select.read().ready(is.fd()))
                      {
                          istream_ready=true;
                      }
                      if(select.read().ready(0))
                      {
                          stdin_stream_ready=true;
                      }
                  }
              }
              else
              {
                  if(!istream_ready)
                  {
                      istream_select.wait(boost::posix_time::milliseconds(10));
                      if(istream_select.read().ready(is.fd()))
                      {
                          istream_ready=true;
                      }
                  }
              }
              #endif //#ifdef WIN32
              //keep storing available bounding data
              if(istream_ready)
              {
                  if(!buffer_size || bounding_queue.size()<*buffer_size || discard_bounding)
                  {
                      const Point* q = istream.read();
                      if( q )
                      {
                          std::string line = csv.binary() ? std::string( istream.binary().last(), csv.format().size() ) : comma::join( istream.ascii().last(), stdin_csv.delimiter );
                          bounding_queue.push_back(std::make_pair(get_time(*q),line));
                      }
                      else
                      {
                          bounding_data_available=false;
                      }
                  }
                  if(buffer_size && bounding_queue.size()>*buffer_size && discard_bounding)
                  {
                      bounding_queue.pop_front();
                  }
              }

              //if we are done with the last bounded point get next
              if(next)
              {
                  if(!stdin_stream_ready) { continue; }
                  p = stdin_stream.read();
                  if( !p ) { break; }
              }

              boost::posix_time::ptime t = get_time(*p);
              
              //get bound
              while(bounding_queue.size()>=2)
              {
                  if( t < bounding_queue[1].first ) { break; }
                  bounding_queue.pop_front();
              }

              if(bounding_queue.size()<2)
              {
                  //bound not found
                  //do we have more data?
                  if(!bounding_data_available) { break; }
                  next=false;
                  continue;
  //                if(bounding_data_available) { next=false; continue; }
  //                if(bounding_queue.empty()) { break; } //no bounding data
  //                if(p->timestamp < bounding_queue.front().first || !by_lower) { break; }
  //                //duplicate point to emulate first
  //                bounding_queue.push_back(bounding_queue.front());
              }

              //bound available

              
              //check late points
              if( (discard || !by_upper) && t < bounding_queue.front().first )
              {
                  //std::cerr<<bounding_queue[0].first<<","<<p->timestamp<<","<<bounding_queue[1].first<<std::endl;
                  next=true;
                  continue;
              }

              bool is_first = by_lower || ( nearest && ( t - bounding_queue[0].first ) < ( bounding_queue[1].first - t ) );

              const timestring_t& chosen_bound = is_first ? bounding_queue[0] : bounding_queue[1];;

              //check bound
              if( bound && !( ( chosen_bound.first - *bound ) <= t && t <= ( chosen_bound.first + *bound ) ) )
              {
                  next=true;
                  continue;
              }

              //match available -> join and output
              if( stdin_csv.binary() )
              {
                  if( bounded_first )
                  {
                      std::cout.write( stdin_stream.binary().last(), stdin_csv.format().size() );
                  }
                  if( select_only ) { } /// This is --do-no-append, so don't append
                  else if( timestamp_only )
                  {
                      static comma::csv::binary< Point > b;
                      std::vector< char > v( b.format().size() );
                      b.put( Point( chosen_bound.first ), &v[0] );
                      std::cout.write( &v[0], b.format().size() );
                  }
                  else
                  {
                      std::cout.write( &chosen_bound.second[0], chosen_bound.second.size() );
                  }
                  if( !bounded_first )
                  {
                      std::cout.write( stdin_stream.binary().last(), stdin_csv.format().size() );
                  }
                  std::cout.flush();
              }
              else
              {
                  if( bounded_first )
                  {
                      std::cout << comma::join( stdin_stream.ascii().last(), stdin_csv.delimiter );
                  }
                  if( select_only ) { } /// This is --do-no-append, so don't append
                  else if( timestamp_only )
                  {
                      std::cout << stdin_csv.delimiter << boost::posix_time::to_iso_string( chosen_bound.first );
                  }
                  else
                  {
                      std::cout << stdin_csv.delimiter << chosen_bound.second;
                  }
                  if( !bounded_first )
                  {
                      std::cout << stdin_csv.delimiter << comma::join( stdin_stream.ascii().last(), stdin_csv.delimiter );
                  }
                  std::cout << std::endl;
              }
              //get a new point
              next=true;
          }
        }
        return 0;     
    }
    catch( std::exception& ex ) { std::cerr << "csv-time-join: " << ex.what() << std::endl; }
    catch( ... ) { std::cerr << "csv-time-join: unknown exception" << std::endl; }
    usage();
}
