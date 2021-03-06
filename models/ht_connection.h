/*
 *  ht_connection.h
 *
 *  This file is part of NEST.
 *
 *  Copyright (C) 2004 The NEST Initiative
 *
 *  NEST is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  NEST is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with NEST.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef HT_CONNECTION_H
#define HT_CONNECTION_H

// Includes from nestkernel:
#include "connection.h"

/* BeginDocumentation
  Name: ht_synapse - Synapse with depression after Hill & Tononi (2005).

  Description:
  This synapse implements the depression model described in [1, p 1678].
  Synaptic dynamics are given by

  P'(t) = ( 1 - P ) / tau_p
  P(T+) = (1 - delta_P) P(T-)   for T : time of a spike
  P(t=0) = 1

  w(t) = w_max * P(t)  is the resulting synaptic weight

  Parameters:
     The following parameters can be set in the status dictionary:
     tauP     double - synaptic vesicle pool recovery time constant [ms]
     delta_P  double - fractional change in vesicle pool on incoming spikes
                       [unitless]
     P        double - current size of the vesicle pool [unitless, 0 <= P <= 1]

  Warning:
  THIS SYNAPSE MODEL HAS NOT BEEN TESTED EXTENSIVELY!

  References:
   [1] S Hill and G Tononi (2005). J Neurophysiol 93:1671-1698.

  Sends: SpikeEvent

  FirstVersion: March 2009
  Author: Hans Ekkehard Plesser, based on markram_synapse
  SeeAlso: ht_neuron, tsodyks_synapse, stdp_synapse, static_synapse
*/

/**
 * Class representing a synapse with Hill short term plasticity.  A
 * suitale Connector containing these connections can be obtained from
 * the template GenericConnector.
 */

namespace nest
{

template < typename targetidentifierT >
class HTConnection : public Connection< targetidentifierT >
{
public:
  typedef CommonSynapseProperties CommonPropertiesType;
  typedef Connection< targetidentifierT > ConnectionBase;

  /**
   * Default Constructor.
   * Sets default values for all parameters. Needed by GenericConnectorModel.
   */
  HTConnection();

  /**
   * Copy constructor.
   * Needs to be defined properly in order for GenericConnector to work.
   */
  HTConnection( const HTConnection& );

  // Explicitly declare all methods inherited from the dependent base
  // ConnectionBase. This avoids explicit name prefixes in all places these
  // functions are used. Since ConnectionBase depends on the template parameter,
  // they are not automatically found in the base class.
  using ConnectionBase::get_delay_steps;
  using ConnectionBase::get_delay;
  using ConnectionBase::get_rport;
  using ConnectionBase::get_target;

  /**
   * Default Destructor.
   */
  virtual ~HTConnection()
  {
  }

  /**
   * Get all properties of this connection and put them into a dictionary.
   */
  virtual void get_status( DictionaryDatum& d ) const;

  /**
   * Set properties of this connection from the values given in dictionary.
   */
  virtual void set_status( const DictionaryDatum& d, ConnectorModel& cm );

  /**
   * Send an event to the receiver of this connection.
   * \param e The event to send
   * \param t_lastspike Point in time of last spike sent.
   * \param cp Common properties to all synapses (empty).
   */
  void send( Event& e,
    thread t,
    double_t t_lastspike,
    const CommonSynapseProperties& cp );

  class ConnTestDummyNode : public ConnTestDummyNodeBase
  {
  public:
    // Ensure proper overriding of overloaded virtual functions.
    // Return values from functions are ignored.
    using ConnTestDummyNodeBase::handles_test_event;
    port
    handles_test_event( SpikeEvent&, rport )
    {
      return invalid_port_;
    }
  };

  void
  check_connection( Node& s,
    Node& t,
    rport receptor_type,
    double_t,
    const CommonPropertiesType& )
  {
    ConnTestDummyNode dummy_target;
    ConnectionBase::check_connection_( dummy_target, s, t, receptor_type );
  }

  //! allows efficient initialization from ConnectorModel::add_connection()
  void
  set_weight( double_t w )
  {
    weight_ = w;
  }

private:
  double_t weight_; //!< synpatic weight

  double_t tau_P_;   //!< [ms] time constant for recovery
  double_t delta_P_; //!< fractional decrease in pool size per spike

  double_t p_; //!< current pool size
};


/**
 * Send an event to the receiver of this connection.
 * \param e The event to send
 * \param p The port under which this connection is stored in the Connector.
 * \param t_lastspike Time point of last spike emitted
 */
template < typename targetidentifierT >
inline void
HTConnection< targetidentifierT >::send( Event& e,
  thread t,
  double_t t_lastspike,
  const CommonSynapseProperties& )
{
  double_t h = e.get_stamp().get_ms() - t_lastspike;
  Node* target = get_target( t );
  // t_lastspike_ = 0 initially

  // propagation t_lastspike -> t_spike, t_lastspike_ = 0 initially, p_ = 1
  p_ = 1 - ( 1 - p_ ) * std::exp( -h / tau_P_ );

  // send the spike to the target
  e.set_receiver( *target );
  e.set_weight( weight_ * p_ );
  e.set_delay( get_delay_steps() );
  e.set_rport( get_rport() );
  e();

  // reduce pool after spike is sent
  p_ *= ( 1 - delta_P_ );
}

template < typename targetidentifierT >
HTConnection< targetidentifierT >::HTConnection()
  : ConnectionBase()
  , weight_( 1.0 )
  , tau_P_( 50.0 )
  , delta_P_( 0.2 )
  , p_( 1.0 )
{
}

template < typename targetidentifierT >
HTConnection< targetidentifierT >::HTConnection( const HTConnection& rhs )
  : ConnectionBase( rhs )
  , weight_( rhs.weight_ )
  , tau_P_( rhs.tau_P_ )
  , delta_P_( rhs.delta_P_ )
  , p_( rhs.p_ )
{
}

template < typename targetidentifierT >
void
HTConnection< targetidentifierT >::get_status( DictionaryDatum& d ) const
{
  ConnectionBase::get_status( d );
  def< double_t >( d, names::weight, weight_ );
  def< double_t >( d, "tau_P", tau_P_ );
  def< double_t >( d, "delta_P", delta_P_ );
  def< double_t >( d, "P", p_ );
  def< long_t >( d, names::size_of, sizeof( *this ) );
}

template < typename targetidentifierT >
void
HTConnection< targetidentifierT >::set_status( const DictionaryDatum& d,
  ConnectorModel& cm )
{
  ConnectionBase::set_status( d, cm );

  updateValue< double_t >( d, names::weight, weight_ );
  updateValue< double_t >( d, "tau_P", tau_P_ );
  updateValue< double_t >( d, "delta_P", delta_P_ );
  updateValue< double_t >( d, "P", p_ );

  if ( tau_P_ <= 0.0 )
    throw BadProperty( "tau_P >= 0 required." );

  if ( delta_P_ < 0.0 || delta_P_ > 1.0 )
    throw BadProperty( "0 <= delta_P <= 1 required." );

  if ( p_ < 0.0 || p_ > 1.0 )
    throw BadProperty( "0 <= P <= 1 required." );
}

} // namespace

#endif // HT_CONNECTION_H
