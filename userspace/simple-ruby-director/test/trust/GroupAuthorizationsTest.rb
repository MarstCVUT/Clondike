require 'test/trust/FunctionalTrustBaseTest.rb'

class GroupAuthorizationsTest<FunctionalTrustBaseTest

  def testLocalAuthorizationAndRevocation
      prepareDirectories
      identity1, identity2, identity3, identity4 = generateIdentities()
      
      trustManagement = TrustManagement.new(identity1, nil)      
      session = trustManagement.authenticate(identity2.publicKey())
      session2 = trustManagement.authenticate(identity3.publicKey())
      
      groupCertificate1 = identity1.issueGroupMembershipCertificate("someGroup", identity2.publicKey)
      authorizationCertificate1 = identity1.issueAuthorizationCertificate(identity1.publicKey, "someGroup", identity1.publicKey, nil, FileSystemPermission.new("read", "/tmp"), false)

      assert trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp/test/west")
      assert !trustManagement.checkPermissionForSession(session2, "fs", "read", "/tmp")
      
      groupCertificate2 = identity1.issueGroupMembershipCertificate("someGroup", identity3.publicKey)
      
      assert trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp/test/west")
      assert trustManagement.checkPermissionForSession(session2, "fs", "read", "/tmp")
      
      identity1.revokeCertificate(groupCertificate1)

      assert !trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp/test/west")
      assert trustManagement.checkPermissionForSession(session2, "fs", "read", "/tmp")

      identity1.revokeCertificate(groupCertificate2)

      assert !trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp/test/west")
      assert !trustManagement.checkPermissionForSession(session2, "fs", "read", "/tmp")      
      
      groupCertificate3 = identity1.issueGroupMembershipCertificate("someGroup", identity3.publicKey)
      
      assert !trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp/test/west")
      assert trustManagement.checkPermissionForSession(session2, "fs", "read", "/tmp")            
      
      identity1.revokeCertificate(authorizationCertificate1)
      
      assert !trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp/test/west")
      assert !trustManagement.checkPermissionForSession(session2, "fs", "read", "/tmp")            
  end
  
  def testRemoteAuthorizationAndRevocation
      prepareDirectories
      identity1, identity2, identity3, identity4 = generateIdentities()
      
      trustManagement = TrustManagement.new(identity1, nil)      
      session = trustManagement.authenticate(identity2.publicKey())
      session2 = trustManagement.authenticate(identity3.publicKey())
      
      groupCertificate1 = identity2.issueGroupMembershipCertificate("someGroup", identity2.publicKey)
      groupCertificate2 = identity2.issueGroupMembershipCertificate("someGroup", identity3.publicKey)
      
      authorizationCertificate1 = identity1.issueAuthorizationCertificate(identity2.publicKey, "someGroup", identity1.publicKey, nil, FileSystemPermission.new("read", "/tmp"), false)

      assert !trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp/test/west")
      assert !trustManagement.checkPermissionForSession(session2, "fs", "read", "/tmp")
            
      identity1.receiveCertificate(groupCertificate1)
      
      assert trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp/test/west")
      assert !trustManagement.checkPermissionForSession(session2, "fs", "read", "/tmp")
      
      identity1.receiveCertificate(groupCertificate2)
      revocation1 = identity2.revokeCertificate(groupCertificate1)      
      
      assert trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp/test/west")
      assert trustManagement.checkPermissionForSession(session2, "fs", "read", "/tmp")      
      
      identity1.receiveCertificate(revocation1)
      
      assert !trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp/test/west")
      assert trustManagement.checkPermissionForSession(session2, "fs", "read", "/tmp")            
      
      revocation2 = identity1.revokeCertificate(authorizationCertificate1)      
      
      assert !trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp/test/west")
      assert !trustManagement.checkPermissionForSession(session2, "fs", "read", "/tmp")                  
  end  

  def testMultistepAuthorizationAndRevocation
      prepareDirectories
      identity1, identity2, identity3, identity4 = generateIdentities()
      
      trustManagement = TrustManagement.new(identity1, nil)      
      session = trustManagement.authenticate(identity3.publicKey())
      session2 = trustManagement.authenticate(identity4.publicKey())
      
      authorizationCertificate1 = identity1.issueAuthorizationCertificate(identity2.publicKey, nil, identity1.publicKey, nil, FileSystemPermission.new("read", "/tmp"), true)
      authorizationCertificate2 = identity2.issueAuthorizationCertificate(identity3.publicKey, nil, identity1.publicKey, nil, FileSystemPermission.new("read", "/tmp"), true)
      
      groupCertificate2 = identity3.issueGroupMembershipCertificate("someGroup2", identity4.publicKey)
      authorizationCertificate3 = identity3.issueAuthorizationCertificate(identity3.publicKey, "someGroup2", identity1.publicKey, nil, FileSystemPermission.new("read", "/tmp"), false)
      
      identity1.receiveCertificate(authorizationCertificate2)
      assert trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp/test/west")
      assert !trustManagement.checkPermissionForSession(session2, "fs", "read", "/tmp")
      
      identity1.receiveCertificate(groupCertificate2)      
      
      assert trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp/test/west")
      assert !trustManagement.checkPermissionForSession(session2, "fs", "read", "/tmp")
      
      identity1.receiveCertificate(authorizationCertificate3)      

      assert trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp/test/west")
      assert trustManagement.checkPermissionForSession(session2, "fs", "read", "/tmp")      
      
      revocation1 = identity1.revokeCertificate(authorizationCertificate1)      
      
      assert !trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp/test/west")
      assert !trustManagement.checkPermissionForSession(session2, "fs", "read", "/tmp")            
  end  
  
    def testRemoteGroupAuthorizationAndRevocation
      prepareDirectories
      identity1, identity2, identity3, identity4 = generateIdentities()

      trustManagement = TrustManagement.new(identity1, nil)      
      session = trustManagement.authenticate(identity2.publicKey())
      session2 = trustManagement.authenticate(identity3.publicKey())
      
      authorizationCertificate1 = identity1.issueAuthorizationCertificate(identity2.publicKey, nil, identity1.publicKey, nil, FileSystemPermission.new("read", "/tmp"), true)
      groupCertificate1 = identity2.issueGroupMembershipCertificate("someGroup", identity1.publicKey)        
      authorizationCertificate2 = identity2.issueAuthorizationCertificate(identity3.publicKey, nil, identity2.publicKey, "someGroup", FileSystemPermission.new("read", "/tmp"), false)
        
        
      assert trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp")
      assert !trustManagement.checkPermissionForSession(session2, "fs", "read", "/tmp")
      
      identity1.receiveCertificate(authorizationCertificate2)
      
      assert trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp")
      assert !trustManagement.checkPermissionForSession(session2, "fs", "read", "/tmp")        
      
      identity1.receiveCertificate(groupCertificate1)

      assert trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp")
      assert trustManagement.checkPermissionForSession(session2, "fs", "read", "/tmp")                
      
      revocation1 = identity2.revokeCertificate(groupCertificate1)      
      identity1.receiveCertificate(revocation1)
      
      assert trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp")
      assert !trustManagement.checkPermissionForSession(session2, "fs", "read", "/tmp")                
    end
  
end
