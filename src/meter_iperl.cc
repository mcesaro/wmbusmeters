/*
 Copyright (C) 2017-2020 Fredrik Öhrström
 Copyright (C) 2018 David Mallon

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include"dvparser.h"
#include"meters.h"
#include"meters_common_implementation.h"
#include"wmbus.h"
#include"wmbus_utils.h"

using namespace std;

struct MeterIperl : public virtual WaterMeter, public virtual MeterCommonImplementation {
    MeterIperl(WMBus *bus, MeterInfo &mi);

    // Total water counted through the meter
    double totalWaterConsumption(Unit u);
    bool  hasTotalWaterConsumption();
    double maxFlow(Unit u);
    bool  hasMaxFlow();

private:
    void processContent(Telegram *t);

    double total_water_consumption_m3_ {};
    double max_flow_m3h_ {};
};

MeterIperl::MeterIperl(WMBus *bus, MeterInfo &mi) :
    MeterCommonImplementation(bus, mi, MeterType::IPERL, MANUFACTURER_SEN)
{
    setExpectedTPLSecurityMode(TPLSecurityMode::AES_CBC_IV);

    addMedia(0x06);
    addMedia(0x07);

    addLinkMode(LinkMode::T1);

    setExpectedVersion(0x68);

    addPrint("total", Quantity::Volume,
             [&](Unit u){ return totalWaterConsumption(u); },
             "The total water consumption recorded by this meter.",
             true, true);

    addPrint("max_flow", Quantity::Flow,
             [&](Unit u){ return maxFlow(u); },
             "The maxium flow recorded during previous period.",
             true, true);
}

unique_ptr<WaterMeter> createIperl(WMBus *bus, MeterInfo &mi)
{
    return unique_ptr<WaterMeter>(new MeterIperl(bus, mi));
}

void MeterIperl::processContent(Telegram *t)
{
    int offset;
    string key;

    if(findKey(MeasurementType::Unknown, ValueInformation::Volume, 0, 0, &key, &t->values)) {
        extractDVdouble(&t->values, key, &offset, &total_water_consumption_m3_);
        t->addMoreExplanation(offset, " total consumption (%f m3)", total_water_consumption_m3_);
    }

    if(findKey(MeasurementType::Unknown, ValueInformation::VolumeFlow, ANY_STORAGENR, 0, &key, &t->values)) {
        extractDVdouble(&t->values, key, &offset, &max_flow_m3h_);
        t->addMoreExplanation(offset, " max flow (%f m3/h)", max_flow_m3h_);
    }
}

double MeterIperl::totalWaterConsumption(Unit u)
{
    assertQuantity(u, Quantity::Volume);
    return convert(total_water_consumption_m3_, Unit::M3, u);
}

bool MeterIperl::hasTotalWaterConsumption()
{
    return true;
}

double MeterIperl::maxFlow(Unit u)
{
    assertQuantity(u, Quantity::Flow);
    return convert(max_flow_m3h_, Unit::M3H, u);
}

bool MeterIperl::hasMaxFlow()
{
    return true;
}
