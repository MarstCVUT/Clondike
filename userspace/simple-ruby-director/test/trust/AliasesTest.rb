require 'test/trust/FunctionalTrustBaseTest'

class AliasesTest<FunctionalTrustBaseTest

  def testAliasIssuing
      prepareDirectories
      identity1, identity2, identity3, identity4 = generateIdentities()
      
      trustManagement = TrustManagement.new(identity1)      
      
      assert trustManagement.dataStore.entities["someAlias"] == nil
      aliasCertificate = identity1.issueAliasCertificate("someAlias", identity2.publicKey)                        
      assert trustManagement.dataStore.entities["someAlias"] != nil
      assert trustManagement.dataStore.entities["someAlias"].entities.size == 1                  
      parentEntity = trustManagement.dataStore.entities["someAlias"].entities.keys.to_a()[0]
      identity2Entity = trustManagement.dataStore.entities[identity2.publicKey]
      assert parentEntity == identity2Entity
      
      begin
        aliasCertificate2 = identity1.issueAliasCertificate("someAlias", identity3.publicKey)      
        fail "Should not get here, an exception should be thrown"
      rescue
      end
        
      
      identity1.revokeCertificate(aliasCertificate)
      assert trustManagement.dataStore.entities["someAlias"] == nil
  end
  
end