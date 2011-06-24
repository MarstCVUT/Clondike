
def prepareExecuteMeasurementPlanParser()
    measureParser = CommandParser.new("measure")
    planFilenameParser = AttributeParser.new("planFilename", true)
    planFilenameParser.setValueParser(StringValueParser.new)
    resultFilenameParser = AttributeParser.new("resultFilename", true)
    resultFilenameParser.setValueParser(StringValueParser.new)
    measureParser.addAttributeParser(planFilenameParser)
    measureParser.addAttributeParser(resultFilenameParser)
    return measureParser
end

def prepareExecuteMultiMeasurementPlanParser()
    measureParser = CommandParser.new("multimeasure")
    planFilenameParser = AttributeParser.new("planFilename", true)
    planFilenameParser.setValueParser(StringValueParser.new)
    pathParser = AttributeParser.new("path", true)
    pathParser.setValueParser(StringValueParser.new)
    measureParser.addAttributeParser(planFilenameParser)
    measureParser.addAttributeParser(pathParser)
    return measureParser
end

# Helper method to register all know measurement cli interpreter handlers
def registerAllMeasurementParsers(parser)    
    parser.addCommandParser(prepareExecuteMeasurementPlanParser())
    parser.addCommandParser(prepareExecuteMultiMeasurementPlanParser())
end
