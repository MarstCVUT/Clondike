
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

# Helper method to register all know measurement cli interpreter handlers
def registerAllMeasurementParsers(parser)    
    parser.addCommandParser(prepareExecuteMeasurementPlanParser())
end
