require 'set'

class NamedAttributesValue
    attr_reader :name
    attr_reader :attributes
    attr_reader :value
    
    def initialize(name)
        @name = name
        @attributes = {}
    end
    
    def addAttributes(attributes)
        attributes.each { |attribute|
            #puts "Adding attribute #{attribute.name} to #{name}"
            @attributes[attribute.name] = attribute
        }
    end
        
    def setValue(value)
        @value = value
    end    
end

class CommandAttribute<NamedAttributesValue
    def initialize(name)
        super(name)
    end
end

class Command<NamedAttributesValue
    def initialize(name)
        super(name)
    end    
end