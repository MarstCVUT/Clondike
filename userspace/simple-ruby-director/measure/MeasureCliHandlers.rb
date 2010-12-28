
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

# Helper method to register all know measurement related cli interpreter handlers
def registerAllMeasureHandlers(measurementPlanParser, measurementDirector, interpreter)
    interpreter.addHandler("measure", MeasureHandler.new(measurementPlanParser, measurementDirector))
end