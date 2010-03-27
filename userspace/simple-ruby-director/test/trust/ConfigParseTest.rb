require 'test/unit'
require 'trust/SecurityConfiguration.rb'
require 'trust/Entity.rb'

class ConfigParseTestTest < Test::Unit::TestCase
  def testConfigLoad
      #puts "#{Dir.pwd}"
      securityConfig = SecurityConfiguration.new()
      securityConfig.load("test/trust/test.conf")
      
      securityConfig.save("test/trust/test-save.cfg")
      reloadedConfig = SecurityConfiguration.new()
      reloadedConfig.load("test/trust/test-save.cfg")
      #securityConfig.entities.each_value { |entity|
      #    puts "Entity: #{entity}"
      #}

      performAssertions(securityConfig)
      performAssertions(reloadedConfig)
  end
  
  def performAssertions(securityConfig)
    assert_equal 16, securityConfig.entities.size, "Count of entities should match"
    assert securityConfig.entities['Delta'] == nil, "Delta is not present"
    assert securityConfig.entities['Gamma'] != nil, "Gamma is present"
    assert securityConfig.entities['Group5'] != nil, "Group5 is present"
    assert securityConfig.entities['Group5'].kind_of?(LocalEntity), "Should be local entity"
    assert securityConfig.entities['Group5'].identities.size == 1, "Should have one element"

    assert securityConfig.entities['Group5'].identities.include?(RemoteGroupEntity.new("LocalGroup2", LocalEntity.new("Beta"))), "Should contain remote group element"
    assert !securityConfig.entities['Group5'].identities.include?(RemoteGroupEntity.new("LocalGroup1", LocalEntity.new("Beta"))), "Should not contain remote group element"
    assert !securityConfig.entities['Group5'].identities.include?(RemoteGroupEntity.new("LocalGroup2", LocalEntity.new("Alfa"))), "Should not contain remote group element"
    
    assert securityConfig.entities['Group2'].kind_of?(LocalEntity), "Should be local entity"
    assert securityConfig.entities['Group2'].identities.size == 2, "Should have two elements"
    assert !securityConfig.entities['Group2'].identities.include?(RemoteGroupEntity.new("LocalGroup2", LocalEntity.new("Beta"))), "Should not contain remote group element"
    assert securityConfig.entities['Group2'].identities.include?(LocalEntity.new("Gamma"))
    assert !securityConfig.entities['Group2'].identities.include?(LocalEntity.new("Alfa"))
    assert securityConfig.entities['Group2'].identities.include?(LocalEntity.new("Group1"))
    assert !securityConfig.entities['Group2'].identities.include?(LocalEntity.new("Group3"))
    #assert(false, 'Assertion was false.')
    #flunk "TODO: Write test"
    # assert_equal("foo", bar)             
  end
end
