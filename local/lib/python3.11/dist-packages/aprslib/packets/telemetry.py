from aprslib.packets.base import APRSPacket

class TelemetryReport(APRSPacket):
    format = 'raw'
    telemetry = dict(seq=0,
                     vals=['0']*6)
    telemetry['vals'][5] = ['1']*8  # Replace io data with list of 8 values
    comment = ''

    def _serialize_body(self):
        # What do we do when len(digitalvalue) != 8?
        tempio = ''.join(self.telemetry['vals'][5])

        body = [
            str(self.telemetry['seq']).zfill(3),
            str(self.telemetry['vals'][0]).zfill(3),
            str(self.telemetry['vals'][1]).zfill(3),
            str(self.telemetry['vals'][2]).zfill(3),
            str(self.telemetry['vals'][3]).zfill(3),
            str(self.telemetry['vals'][4]).zfill(3),
            str(tempio),
            self.comment,
        ]
        # Add packet type to body joined by commas
        return 'T#' + ",".join(body)
