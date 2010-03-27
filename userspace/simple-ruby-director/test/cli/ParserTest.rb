require 'test/unit'
require 'cli/CliParser.rb'

class ParserTest < Test::Unit::TestCase
  def testCorrectAddParse
      parser = prepareParser
      command = parser.parse("add object specification 44 subjectNumber 44 'object value'")
      
      assert command.kind_of?(Command)
      assert command.value() == 'object value'
      assert command.attributes.size == 2
      assertAttribute("add", command.attributes, ["object", nil, [ ["specification", "44", nil] ]])   
      assertAttribute("add", command.attributes, ["subjectNumber", 44, nil])   
      
      command = parser.parse("add object specification 44 verification 'aa vv' 'object value'")
      
      assert command.kind_of?(Command)
      assert command.value() == 'object value'
      assert command.attributes.size == 1
      assertAttribute("add", command.attributes, ["object", nil, [ ["specification", "44", nil], ["verification", "aa vv", nil] ]])   
  end

  def testIncorrectAddParse
      parser = prepareParser      
      begin
        command = parser.parse("ad object")      
        fail "Should fail"
      rescue ParseException => e
          assert_equal e.text, 'Missing required attribute specification.'
      end      
      
      begin
        command = parser.parse("add object specification")      
        fail "Should fail"
      rescue ParseException => e
          assert_equal e.text, "Missing value for attribute 'specification'"
      end            
      
      begin
        command = parser.parse("add object specification 88")      
        fail "Should fail"
      rescue ParseException => e
          assert_equal e.text, "Missing value for attribute 'add'"
      end                  
      
      begin
        command = parser.parse("add object specification 88 subjectNumber dasn")      
        fail "Should fail"
      rescue ParseException => e
          assert_equal e.text, "Invalid number 'dasn'"
      end                        
      
      begin
        command = parser.parse("addx object specification 88 subjectNumber dasn")      
        fail "Should fail"
      rescue ParseException => e
          assert_equal e.text, "Unknown command addx"
      end                        

      begin
        command = parser.parse("add blabla object specification 88 subjectNumber dasn")      
        fail "Should fail"
      rescue ParseException => e
          assert_equal e.text, "Unexpected token 'blabla'. Missing required attribute object."
      end                              
  end
  
  def testCorrectRemoveParse
      parser = prepareParser
      command = parser.parse("remove subject teper")
      
      assert command.kind_of?(Command)
      assert command.value() == nil
      assert command.attributes.size == 1
      assertAttribute("remove", command.attributes, ["subject", 'teper', nil])   
      
      command = parser.parse("remove object subject teper")
      
      assert command.kind_of?(Command)
      assert command.value() == nil
      assert command.attributes.size == 2
      assertAttribute("remove", command.attributes, ["subject", 'teper', nil])
      assertAttribute("remove", command.attributes, ["object", nil, nil])
      
      command = parser.parse("remove object verification subject teper")
      
      assert command.kind_of?(Command)
      assert command.value() == nil
      assert command.attributes.size == 2
      assertAttribute("remove", command.attributes, ["subject", 'teper', nil])
      assertAttribute("remove", command.attributes, ["object", nil, [['verification', nil, nil]] ])      
  end  
  
private
  def assertAttribute(parentName, attributeHash, assertion)
      name = assertion[0]
      value = assertion[1]
      nestedAttributes = assertion[2] != nil ? assertion[2] : []

      #puts "Attribute hash #{attributeHash}"
      attribute = attributeHash[name]
      assert attribute != nil, "Attribute '#{name}' not present in attribute '#{parentName}'"
      assert attribute.name == name
      assert_equal(attribute.value, value)
      assert_equal(attribute.attributes.size, nestedAttributes.size)
      
      nestedAttributes.each { |nestedAttributeAssertion|
          nestedName = nestedAttributeAssertion[0]
          assert attribute.attributes.include?(nestedName)
          nestedAttribute = attribute.attributes[nestedName]
          #puts "NES: #{nestedAttribute} ... name #{nestedAttribute.name}"
          assertAttribute(attribute.name, attribute.attributes, nestedAttributeAssertion)
      }
  end
  
  def prepareParser
      parser = CliParser.new
      
      commandParser1 = CommandParser.new("add")
      commandParser1.setValueParser(StringValueParser.new)
      attributeParser1 = AttributeParser.new("object", true)
      attributeParser2 = AttributeParser.new("specification", true)
      attributeParser3 = AttributeParser.new("verification", false)
      attributeParser4 = AttributeParser.new("subjectNumber", false)
      attributeParser2.setValueParser(StringValueParser.new)
      attributeParser3.setValueParser(StringValueParser.new)
      attributeParser4.setValueParser(IntValueParser.new)      
      attributeParser1.addAttributeParser(attributeParser2)
      attributeParser1.addAttributeParser(attributeParser3)
      commandParser1.addAttributeParser(attributeParser1)
      commandParser1.addAttributeParser(attributeParser4)      
      parser.addCommandParser(commandParser1)
      
      commandParser2 = CommandParser.new("remove")
      attributeParser1 = AttributeParser.new("object", false)
      attributeParser2 = AttributeParser.new("specification", false)
      attributeParser3 = AttributeParser.new("verification", false)
      attributeParser4 = AttributeParser.new("subject", true)
      attributeParser2.setValueParser(StringValueParser.new)
      attributeParser4.setValueParser(StringValueParser.new)      
      attributeParser1.addAttributeParser(attributeParser2)
      attributeParser1.addAttributeParser(attributeParser3)
      commandParser2.addAttributeParser(attributeParser1)
      commandParser2.addAttributeParser(attributeParser4)      
      parser.addCommandParser(commandParser2)
      
      return parser
  end
end
