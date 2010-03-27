require 'test/trust/FunctionalTrustBaseTest.rb'

class AuthorizationsTest<FunctionalTrustBaseTest

  def testLocalAuthorizationAndRevocation
      prepareDirectories
      identity1, identity2, identity3, identity4 = generateIdentities()
      
      trustManagement = TrustManagement.new(identity1, nil)      
      session = trustManagement.authenticate(identity2.publicKey())
      session2 = trustManagement.authenticate(identity3.publicKey())
      assert !trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp")
      assert !trustManagement.checkPermissionForSession(session2, "fs", "read", "/tmp")
      
      authorizationCertificate1 = identity1.issueAuthorizationCertificate(identity2.publicKey, nil, identity1.publicKey, nil, FileSystemPermission.new("read", "/tmp"), false)
      assert trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp")
      assert trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp/test/west")
      assert !trustManagement.checkPermissionForSession(session, "fs", "read", "/var")
      assert !trustManagement.checkPermissionForSession(session2, "fs", "read", "/tmp")
      
      identity1.revokeCertificate(authorizationCertificate1)
      assert !trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp")
      #b = Time.now.usec
      assert !trustManagement.checkPermissionForSession(session2, "fs", "read", "/tmp")      
      #e = Time.now.usec
      
      #puts "Duration: #{e-b}"
  end

  def testDelegatedAuthorizationAndRevocation
      prepareDirectories
      identity1, identity2, identity3, identity4 = generateIdentities()
      
      trustManagement = TrustManagement.new(identity1, nil)      
      session = trustManagement.authenticate(identity2.publicKey())
      session2 = trustManagement.authenticate(identity3.publicKey())
      
      authorizationCertificate1 = identity1.issueAuthorizationCertificate(identity2.publicKey, nil, identity1.publicKey, nil, FileSystemPermission.new("read", "/tmp"), true)
      authorizationCertificate2 = identity2.issueAuthorizationCertificate(identity3.publicKey, nil, identity1.publicKey, nil, FileSystemPermission.new("read", "/tmp"), true)
      
      assert !trustManagement.checkPermissionForSession(session2, "fs", "read", "/tmp")
      
      identity1.receiveCertificate(authorizationCertificate2)
      assert trustManagement.checkPermissionForSession(session2, "fs", "read", "/tmp")      
      
      revocationCertificate1 = identity2.revokeCertificate(authorizationCertificate2)
      assert trustManagement.checkPermissionForSession(session2, "fs", "read", "/tmp")

      identity1.receiveCertificate(revocationCertificate1)
      assert !trustManagement.checkPermissionForSession(session2, "fs", "read", "/tmp")
  end

  def testMultistepDelegatedAuthorizationAndRevocation
      prepareDirectories
      identity1, identity2, identity3, identity4 = generateIdentities()
      
      trustManagement = TrustManagement.new(identity1, nil)      
      session = trustManagement.authenticate(identity4.publicKey())
      
      authorizationCertificate1 = identity1.issueAuthorizationCertificate(identity2.publicKey, nil, identity1.publicKey, nil, FileSystemPermission.new("read", "/tmp"), true)
      authorizationCertificate2 = identity2.issueAuthorizationCertificate(identity3.publicKey, nil, identity1.publicKey, nil, FileSystemPermission.new("read", "/tmp"), true)
      authorizationCertificate3 = identity3.issueAuthorizationCertificate(identity4.publicKey, nil, identity1.publicKey, nil, FileSystemPermission.new("read", "/tmp"), false)
      
      assert !trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp")

      identity1.receiveCertificate(authorizationCertificate3)
      assert !trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp")
      
      identity1.receiveCertificate(authorizationCertificate2)
      assert trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp")
      
      revocationCertificate1 = identity1.revokeCertificate(authorizationCertificate2)
      assert !trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp")
  end

  def testMultistepDelegatedAuthorizationAndIndirectRevocation
      prepareDirectories
      identity1, identity2, identity3, identity4 = generateIdentities()
      
      trustManagement = TrustManagement.new(identity1, nil)      
      session = trustManagement.authenticate(identity4.publicKey())
      
      authorizationCertificate1 = identity1.issueAuthorizationCertificate(identity2.publicKey, nil, identity1.publicKey, nil, FileSystemPermission.new("read", "/tmp"), true)
      authorizationCertificate2 = identity2.issueAuthorizationCertificate(identity3.publicKey, nil, identity1.publicKey, nil, FileSystemPermission.new("read", "/tmp"), true)
      authorizationCertificate3 = identity3.issueAuthorizationCertificate(identity4.publicKey, nil, identity1.publicKey, nil, FileSystemPermission.new("read", "/tmp"), false)
      
      assert !trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp")

      identity1.receiveCertificate(authorizationCertificate3)
      assert !trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp")
      
      identity1.receiveCertificate(authorizationCertificate2)
      assert trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp")
      
      # This is invalid revocation and should not revoke anything!
      nonsenseRevocation = identity4.revokeCertificate(authorizationCertificate2)
      identity1.receiveCertificate(nonsenseRevocation)
      
      revocationCertificate1 = identity2.revokeCertificate(authorizationCertificate2)
      assert trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp")
      
      identity1.receiveCertificate(revocationCertificate1)
      assert !trustManagement.checkPermissionForSession(session, "fs", "read", "/tmp")
  end   
end