#ifndef OPCODES_H
#define OPCODES_H

enum Opcodes {
    OP_USER_REGISTER                                = 0x001,
    OP_USER_LOGIN                                   = 0x002,

    OP_USER_REGISTER_SUCCESS                        = 0x100,

    OP_USER_REGISTER_LOGIN_TAKEN                    = 0x200,

    OP_ERROR                                        = 0x666,
};

#endif // OPCODES_H
