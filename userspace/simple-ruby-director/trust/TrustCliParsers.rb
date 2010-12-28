

def prepareAliasCommandParser()
    commandParser = CommandParser.new("alias")
    aliasNameParser = AttributeParser.new("name", true)
    aliasNameParser.setValueParser(StringValueParser.new)
    keyNameParser = AttributeParser.new("key", true)
    keyNameParser.setValueParser(PublicKeyValueParser.new)
    commandParser.addAttributeParser(aliasNameParser)
    commandParser.addAttributeParser(keyNameParser)
    return commandParser
end

def prepareDealiasCommandParser()
    commandParser = CommandParser.new("dealias")
    aliasNameParser = AttributeParser.new("name", true)
    aliasNameParser.setValueParser(StringValueParser.new)
    commandParser.addAttributeParser(aliasNameParser)
    return commandParser
end

def prepareMemberCommandParser()
    commandParser = CommandParser.new("member")
    aliasNameParser = AttributeParser.new("group", true)
    aliasNameParser.setValueParser(StringValueParser.new)
    keyNameParser = AttributeParser.new("key", true)
    keyNameParser.setValueParser(PublicKeyValueParser.new)
    commandParser.addAttributeParser(aliasNameParser)
    commandParser.addAttributeParser(keyNameParser)
    return commandParser
end

def prepareDememberCommandParser()
    commandParser = CommandParser.new("demember")
    aliasNameParser = AttributeParser.new("group", true)
    aliasNameParser.setValueParser(StringValueParser.new)
    keyNameParser = AttributeParser.new("key", true)
    keyNameParser.setValueParser(PublicKeyValueParser.new)
    commandParser.addAttributeParser(aliasNameParser)
    commandParser.addAttributeParser(keyNameParser)
    return commandParser
end

def prepareAuthorizeCommandParser()
    commandParser = CommandParser.new("authorize")
    
    authorizeeParser = AttributeParser.new("authorizee", true)
    # TODO: We do not have proper required if support.. we'd need some require this or that support..
    authorizeeNameParser = AttributeParser.new("name", false)
    authorizeeNameParser.setValueParser(StringValueParser.new)
    authorizeeKeyParser = AttributeParser.new("key", false)
    authorizeeKeyParser.setValueParser(PublicKeyValueParser.new)
    authorizeeGroupParser = AttributeParser.new("group", false)
    authorizeeGroupParser.setValueParser(StringValueParser.new)
    authorizeeParser.addAttributeParser(authorizeeNameParser)
    authorizeeParser.addAttributeParser(authorizeeKeyParser)
    authorizeeParser.addAttributeParser(authorizeeGroupParser)

    targetParser = AttributeParser.new("target", true)
    # TODO: We do not have proper required if support.. we'd need some require this or that support..
    targetNameParser = AttributeParser.new("name", false)
    targetNameParser.setValueParser(StringValueParser.new)
    targetKeyParser = AttributeParser.new("key", false)
    targetKeyParser.setValueParser(PublicKeyValueParser.new)
    targetGroupParser = AttributeParser.new("group", false)
    targetGroupParser.setValueParser(StringValueParser.new)
    targetParser.addAttributeParser(targetNameParser)
    targetParser.addAttributeParser(targetKeyParser)
    targetParser.addAttributeParser(targetGroupParser)
    
    permissionParser = AttributeParser.new("permission", true)
    permissionTypeParser = AttributeParser.new("type", true)
    permissionTypeParser.setValueParser(StringValueParser.new)
    permissionOperationParser = AttributeParser.new("operation", true)
    permissionOperationParser.setValueParser(StringValueParser.new)
    permissionObjectParser = AttributeParser.new("object", true)
    permissionObjectParser.setValueParser(StringValueParser.new)
    permissionDelegateParser = AttributeParser.new("delegate", false)
    permissionBackPropagateParser = AttributeParser.new("backPropagate", false)
    permissionParser.addAttributeParser(permissionTypeParser)
    permissionParser.addAttributeParser(permissionOperationParser)
    permissionParser.addAttributeParser(permissionObjectParser)
    permissionParser.addAttributeParser(permissionDelegateParser)
    permissionParser.addAttributeParser(permissionBackPropagateParser)
    
    commandParser.addAttributeParser(authorizeeParser)
    commandParser.addAttributeParser(targetParser)
    commandParser.addAttributeParser(permissionParser)
    return commandParser
end

def prepareDeauthorizeCommandParser()
    commandParser = CommandParser.new("deauthorize")
    
    authorizeeParser = AttributeParser.new("authorizee", true)
    # TODO: We do not have proper required if support.. we'd need some require this or that support..
    authorizeeNameParser = AttributeParser.new("name", false)
    authorizeeNameParser.setValueParser(StringValueParser.new)
    authorizeeKeyParser = AttributeParser.new("key", false)
    authorizeeKeyParser.setValueParser(PublicKeyValueParser.new)
    authorizeeGroupParser = AttributeParser.new("group", false)
    authorizeeGroupParser.setValueParser(StringValueParser.new)
    authorizeeParser.addAttributeParser(authorizeeNameParser)
    authorizeeParser.addAttributeParser(authorizeeKeyParser)
    authorizeeParser.addAttributeParser(authorizeeGroupParser)

    targetParser = AttributeParser.new("target", true)
    # TODO: We do not have proper required if support.. we'd need some require this or that support..
    targetNameParser = AttributeParser.new("name", false)
    targetNameParser.setValueParser(StringValueParser.new)
    targetKeyParser = AttributeParser.new("key", false)
    targetKeyParser.setValueParser(PublicKeyValueParser.new)
    targetGroupParser = AttributeParser.new("group", false)
    targetGroupParser.setValueParser(StringValueParser.new)
    targetParser.addAttributeParser(targetNameParser)
    targetParser.addAttributeParser(targetKeyParser)
    targetParser.addAttributeParser(targetGroupParser)
    
    permissionParser = AttributeParser.new("permission", true)
    permissionTypeParser = AttributeParser.new("type", true)
    permissionTypeParser.setValueParser(StringValueParser.new)
    permissionOperationParser = AttributeParser.new("operation", true)
    permissionOperationParser.setValueParser(StringValueParser.new)
    permissionObjectParser = AttributeParser.new("object", true)
    permissionObjectParser.setValueParser(StringValueParser.new)    
    permissionParser.addAttributeParser(permissionTypeParser)
    permissionParser.addAttributeParser(permissionOperationParser)
    permissionParser.addAttributeParser(permissionObjectParser)
    
    commandParser.addAttributeParser(authorizeeParser)
    commandParser.addAttributeParser(targetParser)
    commandParser.addAttributeParser(permissionParser)
    return commandParser
end


def prepareShowCommandParser
    commandParser = CommandParser.new("show")
    
    showNodeParser = AttributeParser.new("node", false)
    showNodeNameParser = AttributeParser.new("name", false)
    showNodeNameParser.setValueParser(StringValueParser.new)
    showNodeKeyParser = AttributeParser.new("key", false)
    showNodeKeyParser.setValueParser(PublicKeyValueParser.new)
    showNodeParser.addAttributeParser(showNodeNameParser)
    showNodeParser.addAttributeParser(showNodeKeyParser)

    showGroupParser = AttributeParser.new("group", false)
    showGroupNameParser = AttributeParser.new("name", true)
    showGroupNameParser.setValueParser(StringValueParser.new)
    showGroupKeyParser = AttributeParser.new("key", false)
    showGroupKeyParser.setValueParser(PublicKeyValueParser.new)
    showGroupParser.addAttributeParser(showGroupNameParser)
    showGroupParser.addAttributeParser(showGroupKeyParser)
    
    showAllParser = AttributeParser.new("all", false)
    
    commandParser.addAttributeParser(showNodeParser)
    commandParser.addAttributeParser(showGroupParser)
    commandParser.addAttributeParser(showAllParser)    
    
    return commandParser
end

def prepareCheckPermissionCommandParser()
    commandParser = CommandParser.new("check")
    
    authorizeeParser = AttributeParser.new("entity", true)
    # TODO: We do not have proper required if support.. we'd need some require this or that support..
    authorizeeNameParser = AttributeParser.new("name", false)
    authorizeeNameParser.setValueParser(StringValueParser.new)
    authorizeeKeyParser = AttributeParser.new("key", false)
    authorizeeKeyParser.setValueParser(PublicKeyValueParser.new)
    authorizeeGroupParser = AttributeParser.new("group", false)
    authorizeeGroupParser.setValueParser(StringValueParser.new)
    authorizeeParser.addAttributeParser(authorizeeNameParser)
    authorizeeParser.addAttributeParser(authorizeeKeyParser)
    authorizeeParser.addAttributeParser(authorizeeGroupParser)
    
    permissionParser = AttributeParser.new("permission", true)
    permissionTypeParser = AttributeParser.new("type", true)
    permissionTypeParser.setValueParser(StringValueParser.new)
    permissionOperationParser = AttributeParser.new("operation", true)
    permissionOperationParser.setValueParser(StringValueParser.new)
    permissionObjectParser = AttributeParser.new("object", true)
    permissionObjectParser.setValueParser(StringValueParser.new)
    permissionDelegateParser = AttributeParser.new("delegate", false)
    permissionParser.addAttributeParser(permissionTypeParser)
    permissionParser.addAttributeParser(permissionOperationParser)
    permissionParser.addAttributeParser(permissionObjectParser)
    permissionParser.addAttributeParser(permissionDelegateParser)
    
    commandParser.addAttributeParser(authorizeeParser)
    commandParser.addAttributeParser(permissionParser)
    return commandParser
end


# Helper method to register all know trust related cli interpreter handlers
def registerAllTrustParsers(parser)    
    parser.addCommandParser(prepareAliasCommandParser())
    parser.addCommandParser(prepareDealiasCommandParser())
    parser.addCommandParser(prepareMemberCommandParser())
    parser.addCommandParser(prepareDememberCommandParser())
    parser.addCommandParser(prepareAuthorizeCommandParser())    
    parser.addCommandParser(prepareDeauthorizeCommandParser())
    parser.addCommandParser(prepareShowCommandParser())    
    parser.addCommandParser(prepareCheckPermissionCommandParser())    
end