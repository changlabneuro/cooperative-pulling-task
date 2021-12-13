import lever
import time

PORT = 'COM3'

def test_read_state(ctx: lever.SerialContext):
    t0 = time.time()
    while time.time() - t0 < 5.0:
        success, data = lever.read_state(ctx)
        print('Success ? {}; data: {}'.format(success, data))
        if not success:
            continue
        success, strain_read, calc_pwm, actual_pwm = lever.parse_read_state(data)
        print('Success ? {}; strain read: {}; calc_pwm: {}; actual_pwm: {}\n'.format(
            success, strain_read, calc_pwm, actual_pwm))

def test_set_force(ctx: lever.SerialContext):
    t0 = time.time()
    while time.time() - t0 < 5.0:
        success, response = lever.set_force_grams(ctx, 10)
        print('Success ? {}; response: {}'.format(success, response))

if __name__ == '__main__':
    err, ctx = lever.make_default_opened_context(PORT)
    if err is not None:
        raise RuntimeError(err)

    test_read_state(ctx)
    test_set_force(ctx)
    ctx.close()