require 'measure/MeasureMessages.rb'
require 'measure/MeasureMessageHandlers.rb'

class MeasurementDirector
  attr_reader :currentMeasurement
  
  def initialize(localNodeId, interconnection)
    @localNodeId = localNodeId
    @interconnection = interconnection
    @interconnection.addReceiveHandler(ExecutionResultMessage, ExecutionResultHandler.new(self)) if ( interconnection )
    @interconnection.addReceiveHandler(ExecutionPlanMessage, ExecutionPlanHandler.new(localNodeId, interconnection)) if ( interconnection )    
    @currentMeasurement = nil
  end
  
  def startMeasurement(measurement)
    @currentMeasurement = measurement
    measurementMessage = ExecutionPlanMessage.new(@localNodeId, measurement.measurementPlan)    
    $log.debug "Distributing execution plan"
    @interconnection.dispatch(nil, measurementMessage) if @interconnection != nil
    measurement.executeLocally(@localNodeId)
  end	
end