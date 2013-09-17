def depth_argument(value, name):
    if value < 3:
        raise ValueError("The {} argument has to be <= 3, you gave : {}"
                         .format(name, value))
    return int(value)


def true_false(value, name):
    if value == "true":
        return True
    elif value == "false":
        return False
    else:
        raise ValueError("The {} argument must be true or false, you gave : {}"
                         .format(name, value))

def option_value(values):
    def to_return(value, name):
        if not value in values:
            error = "The {} argument must be in list"+str(values)
            error +=", you gave {}".format(name, value)
            raise ValueError(error)
        return value
    return to_return

