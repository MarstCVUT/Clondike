
class MeasureHandler
    def initialize(measurementPlanParser, measurementDirector)
      @measurementPlanParser = measurementPlanParser
      @measurementDirector = measurementDirector
    end
    
    def handle(command)
        fileName = command.attributes['planFilename'].value
	resultFileName = command.attributes['resultFilename'].value
	#TODO: Configurable offset.. now start scheduled 3 seconds since "now" to give nodes time to get the plan
	startTime = Time.now.to_f + 3
	begin
	  measurement = @measurementPlanParser.initializeMeasurement(fileName, startTime, resultFileName)
	  @measurementDirector.startMeasurement(measurement)
	rescue Errno::ENOENT 
	  return "Plan file does not exist: #{$!}"
	end
	
        return "Measurement started. Result will ba saved in file #{resultFileName}"
    end
end

class MultiMeasureHandler
    def initialize(measurementPlanParser, measurementDirector)
      @measurementPlanParser = measurementPlanParser
      @measurementDirector = measurementDirector
    end
    
    def handle(command)
        fileName = command.attributes['planFilename'].value
	path = command.attributes['path'].value
	
	IO.foreach(fileName) do |line| 
	      plan, resultFileName = line.split(",")
	      resultFileName = "/tmp/dummy" if !resultFileName
	      plan.strip!
	      resultFileName.strip!
	      
	      next if plan.empty? # Ignore empty lines in config
	      startTime = Time.now.to_f + 3
	      fileName = "#{path}/#{plan}"	      
	      measurement = @measurementPlanParser.initializeMeasurement(fileName, startTime, resultFileName)
	      @measurementDirector.startMeasurement(measurement)
	      @measurementDirector.waitForMeasurementFinished()
	      # Just a short safety period
	      sleep(2)
	end	
    end
end

# Helper method to register all know measurement related cli interpreter handlers
def registerAllMeasureHandlers(measurementPlanParser, measurementDirector, interpreter)
    interpreter.addHandler("measure", MeasureHandler.new(measurementPlanParser, measurementDirector))
    interpreter.addHandler("multimeasure", MultiMeasureHandler.new(measurementPlanParser, measurementDirector))
end