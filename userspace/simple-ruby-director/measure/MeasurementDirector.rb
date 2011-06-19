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
  
  def notifyUpdate(node, nodeInfo)
    if ( @currentMeasurement )
      @currentMeasurement.measurementPlan.addNodeUsageRecord(node.id, nodeInfo.timestamp, nodeInfo.cpuUsage, nodeInfo.load, nodeInfo.immigratedTasks)
    end
  end
  
  def notifyChange(localNodeInfoWithId)
    if ( @currentMeasurement )
      nodeInfo = localNodeInfoWithId.nodeInfo
      @currentMeasurement.measurementPlan.addNodeUsageRecord(localNodeInfoWithId.nodeId, nodeInfo.timestamp, nodeInfo.cpuUsage, nodeInfo.load, nodeInfo.immigratedTasks)
    end    
  end
end