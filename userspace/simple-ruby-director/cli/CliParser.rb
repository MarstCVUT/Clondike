require 'Util.rb'
require 'set'
require 'cli/Command.rb'
require 'cli/Tokenizer.rb'

class ParseException<RuntimeError
    attr_reader :text
    
    def initialize(text)
        @text = text
    end
    
    def to_s
        "#{text}"
    end
end

class ParseContext
    attr_reader :tokenStream
    
    def initialize(tokenStream)
        @tokenStream = tokenStream
    end
end

class StringValueParser
    def parse(parseContext)
       return parseContext.tokenStream.pop 
    end
end

class IntValueParser
    def parse(parseContext)
        value = parseContext.tokenStream.pop
        begin
          return Integer(value)
        rescue
            raise ParseException.new("Invalid number '#{value}'")
        end        
    end
end

class PublicKeyValueParser
    def parse(parseContext)
        value = parseContext.tokenStream.pop
        begin
          return RSAPublicKey.undecoratedLoad(value)
        rescue
            raise ParseException.new("Invalid public key '#{value}'")
        end        
    end
end

class NamedParserWithAttributes
    attr_reader :name
    
    def initialize(name)
        @name = name
        # Key - Attribute name
        # Value - AttributeParser of that attribute
        @attributeParsers = {}        
    end
    
    def addAttributeParser(attributeParser)
        @attributeParsers[attributeParser.name] = attributeParser
    end    
    
    def setValueParser(valueParser)
        # Value parser is optional. In case it is set, value is parsed as a LAST element after all attributes
        # Note, that the value MUST be present in case the parser is specified
        @valueParser = valueParser
    end    

protected
    def parseAttributes(parseContext)
        attributes = Set.new
        alreadyParsedAttributes = Set.new        
        
        while (!parseContext.tokenStream.empty?) do
          attributeToken = parseContext.tokenStream.peak
          #puts "TOKEN #{attributeToken}"
          appliableParsers = resolvePrefixedElement(attributeToken, @attributeParsers)          
          #puts "APPLIABLE PARSERS #{appliableParsers}"
          break if ( appliableParsers.empty? )
          # TODO: This is quite ugly isn't it? What if the token is actually not for this context? We should simply ignore it in this case...
          raise ParseException.new("Ambigious command #{attributeToken}") if ( appliableParsers.size > 1 )          
          raise ParseException.new("Duplicate attribute #{attributeToken}") if ( alreadyParsedAttributes.include?(attributeToken))
          attributeParser = appliableParsers.singleElement[1]
          alreadyParsedAttributes.add(attributeParser.name)          
          attributes.add(attributeParser.parse(parseContext))
        end

        checkAllRequiredAttributesFilled(parseContext, alreadyParsedAttributes)
        return attributes
    end
    
    def checkAllRequiredAttributesFilled(parseContext, alreadyParsedAttributes)        
        nextToken = parseContext.tokenStream.empty? ? nil : parseContext.tokenStream.peak
        errorTextStart = nextToken == nil ? "" : "Unexpected token '#{nextToken}'. "
        @attributeParsers.each_value { |attributeParser|
            raise ParseException.new("#{errorTextStart}Missing required attribute #{attributeParser.name}.") if ( attributeParser.required and !alreadyParsedAttributes.include?(attributeParser.name))
        }
    end
    
    def parseValue(parseContext)
        return if !@valueParser 
        raise ParseException.new("Missing value for attribute '#{name}'") if ( parseContext.tokenStream.empty? )
        return @valueParser.parse(parseContext)
    end  
end

class AttributeParser<NamedParserWithAttributes
    attr_reader :required
    
    def initialize(name, required)
        super(name)
        @required = required
    end

    def parse(parseContext) 
        nameToken = parseContext.tokenStream.pop
        attribute = CommandAttribute.new(@name)
        
        attribute.addAttributes(parseAttributes(parseContext))
        attribute.setValue(parseValue(parseContext))
        
        return attribute
    end    
end

class CommandParser<NamedParserWithAttributes
    
    def initialize(name)
        super(name)
    end
       
    def parse(parseContext) 
        nameToken = parseContext.tokenStream.pop
        command = Command.new(@name)
        
        command.addAttributes(parseAttributes(parseContext))
        command.setValue(parseValue(parseContext))
        
        return command
    end
end

class CliParser
    def initialize
        @commandParsers = {}
    end
    
    def addCommandParser(commandParser)
        @commandParsers[commandParser.name] = commandParser
    end
    
    def parse(text) 
        parseContext = ParseContext.new(TokenStream.new(text))
        firstToken = parseContext.tokenStream.peak
        appliableParsers = resolvePrefixedElement(firstToken, @commandParsers)
        raise ParseException.new("Unknown command #{firstToken}") if ( appliableParsers.empty? )
        raise ParseException.new("Ambigious command #{firstToken}") if ( appliableParsers.size > 1 )
        parseResult = appliableParsers.singleElement[1].parse(parseContext)
        # Check if there were some unprocessed tokens
        raise ParseException.new("Unexpected token #{parseContext.tokenStream.pop}") if !parseContext.tokenStream.empty?
        return parseResult
    end
end

# Takes as input a string prefix and hash of (String, AnyObject).
# excludeNames is an optional set, with keys to exclude from resolving
# Returns a hash containing only those elements whole key starts with prefix
def resolvePrefixedElement(prefix, elementHash, excludeNames = Set.new)
    result = {}
    elementHash.each { |pair|
        next if excludeNames.include?(pair[0])
        result[pair[0]] = pair[1] if ( pair[0].startsWith(prefix))
    }
    return result
end