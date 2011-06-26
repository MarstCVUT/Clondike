require 'measure/MeasureMessages.rb'
require 'measure/MeasureMessageHandlers.rb'

class MeasurementDirector
  attr_reader :currentMeasurement
  
  def initialize(localNodeId, interconnection, measurementAcceptLimiter)
    @localNodeId = localNodeId
    @interconnection = interconnection
    @interconnection.addReceiveHandler(ExecutionResultMessage, ExecutionResultHandler.new(self)) if ( interconnection )
    @interconnection.addReceiveHandler(ExecutionPlanMessage, ExecutionPlanHandler.new(localNodeId, interconnection, self)) if ( interconnection )    
    @interconnection.addReceiveHandler(MeasurementFinished, MeasurementFinishedHandler.new(self)) if ( interconnection )    
    @measurementAcceptLimiter = measurementAcceptLimiter
    @currentMeasurement = nil
    
    # Used for waiting for execution termination
    @monitor = Monitor.new
    @condition = @monitor.new_cond
  end
  
  def startMeasurement(measurement)
    raise "Cannot start measurement while other measurement is in progress!" if @currentMeasurement
    @currentMeasurement = measurement    
    measurementMessage = ExecutionPlanMessage.new(@localNodeId, measurement.measurementPlan, measurement.nodesToBlock)    
    $log.debug "Distributing execution plan"
    @interconnection.dispatch(nil, measurementMessage) if @interconnection != nil
    measurement.registerFinishCallback(self)
    measurement.executeLocally(@localNodeId)
  end	
  
  def measurementFinished(measurement)
    $log.debug "Measurement finished, going to resume all suspended nodes"
    @interconnection.dispatch(nil, MeasurementFinished.new) if @interconnection != nil
    
    @monitor.synchronize {
	    @condition.broadcast()
	    @currentMeasurement = nil
    }    
  end
  
  def waitForMeasurementFinished()
    @monitor.synchronize {
	    @condition.wait() if @currentMeasurement
    }            
  end
  
  def notifyUpdate(node, nodeInfo)
    if ( @currentMeasurement )
      @currentMeasurement.measurementPlan.addNodeUsageRecord(node.id, nodeInfo.timestamp, nodeInfo.cpuUsage, nodeInfo.load, nodeInfo.immigratedTasks, nodeInfo.localTasks)
    end
  end
  
  def notifyChange(localNodeInfoWithId)
    if ( @currentMeasurement )
      nodeInfo = localNodeInfoWithId.nodeInfo
      @currentMeasurement.measurementPlan.addNodeUsageRecord(localNodeInfoWithId.nodeId, nodeInfo.timestamp, nodeInfo.cpuUsage, nodeInfo.load, nodeInfo.immigratedTasks, nodeInfo.localTasks)
    end    
  end
  
  def suspendNode()
    @measurementAcceptLimiter.blocking = true
  end
  
  def resumeNode()
    @measurementAcceptLimiter.blocking = false
  end  
end