#include "vptranslator/vptranslator.h"

bool MakeTranslation::week::operator==(week i2){
    return (i2.weeknumber == weeknumber);
}

bool MakeTranslation::week::operator!=(week i2){
    return (i2.weeknumber != weeknumber);
}

MakeTranslation::MakeTranslation(){
}

bool MakeTranslation::initcs(boost::gregorian::date beginningday, std::string requestedcs){
  std::size_t found;

  found = requestedcs.find_last_of('1');
  //on recherche le dernier jour de validité de la condition de service
  if(found==std::string::npos){
    return false;
  } else {
      enddate = beginningday + boost::gregorian::date_duration(found);
      //on recherche le premier jour de validité
      found = requestedcs.find('1');
      startdate = beginningday + boost::gregorian::date_duration(found);
      CS = requestedcs.substr(found, (enddate - startdate).days() + 1);
      return true;
 }
}

int MakeTranslation::getnextmonday(boost::gregorian::date datetocompare, short sens){
    if (sens == 1){
        boost::gregorian::first_day_of_the_week_after fdaf(boost::gregorian::Monday);
        boost::gregorian::date d = fdaf.get_date(datetocompare);
        return (d - datetocompare).days();

    } else {
        boost::gregorian::first_day_of_the_week_before fdbf(boost::gregorian::Monday);
        boost::gregorian::date d = fdbf.get_date(datetocompare);
        return (d - datetocompare).days();
    }
}

void MakeTranslation::splitcs(){
    target_map.clear();
    week_vector.clear();
    week_vector.reserve((CS.length() / 7) + 1);
//    on recherche la position du premier lundi dans la chaine
    std::string substr;
    short int weeknumber = 0;
    //pos represente la position au sein de la condition de service
    size_t pos = getnextmonday(startdate, 1);
    size_t precpos = 0;
    boost::gregorian::date weekstartdate = startdate;
    week currentweek;

    while (precpos < CS.length()){
    //tant que le prochain lundi est inférieur à enddate, on découpe le régime
        if(pos < CS.length()){
            substr = CS.substr(precpos, pos - precpos);
        } else {
            //gestion de la fin de la cs lorsqu'elle n'est pas une semaine complète
            substr = "0000000";
            if(precpos != 0){
                substr.replace(0, CS.length() - precpos, CS.substr(precpos, CS.length() - precpos));
            //gestion de la cs lorsqu'elle n'est pas une semaine complète
            } else {
                substr.replace(7 - pos, CS.length() - precpos, CS.substr(precpos, CS.length() - precpos));
            }
        };
        currentweek.weeknumber = weeknumber;
        currentweek.startdate = weekstartdate;
        //initialisation du bitset
        currentweek.week_bs = std::bitset<7>(std::string(substr));
        week_vector.push_back(currentweek);

        std::map<int, target>::iterator it;
        it = target_map.find(currentweek.week_bs.to_ulong());
        if (it != target_map.end()){
            it->second.count++;
            it->second.lastweeknumber = weeknumber;
        } else {
            target& response = target_map[currentweek.week_bs.to_ulong()];
            response.week_bs = week_vector[weeknumber].week_bs;
            response.firstweeknumber = weeknumber;
            response.count = 1;
        };

        weekstartdate = weekstartdate + boost::gregorian::date_duration(pos - precpos);
        weeknumber++;
        precpos = pos;
        pos = pos + 7;
    }
}


//calcul des exception ET
//on essaye d'inserer le bitset b2 dans b1
std::bitset<7> MakeTranslation::getandpattern(std::bitset<7>& b1, std::bitset<7>& b2){
    std::bitset<7> dest = b1;
//    std::bitset<7> src = b2;
    std::bitset<7> result = dest.flip() &= b2;
    return result;
}


//calcul des exception OU
//on essaye d'inserer le bitset b2 dans b1
std::bitset<7> MakeTranslation::getexceptpattern(std::bitset<7>& b1, std::bitset<7>& b2){
    std::bitset<7> dest = b1;
    std::bitset<7> result;
    result = (dest ^= b2 ) &= b1;
    return result;
}


void MakeTranslation::bounddrawdown(){
    if(week_vector.size() > 1){
        // la condition de service ne débute pas un lundi, on va rabattre la borne inférieure sur la semaine consécutive
        week startweek = week_vector.front();
        long int startweekkey = startweek.week_bs.to_ulong();
        if (startweek.startdate.day_of_week() != boost::date_time::Monday){
        //on récupere la première semaine complète afin d'inclure la borne basse à cette semaine
            week firstweek = week_vector[1];
           //on ne garde que la partie commune aux 2 semaines
            int weeklength = getnextmonday(startweek.startdate, 1);
            target& responsefw = target_map[firstweek.week_bs.to_ulong()];
            //gestion des ET
            std::bitset<7> exception =getandpattern(firstweek.week_bs, startweek.week_bs);

            if(exception.any()){
                for(int it=0; it!= weeklength; it++) {
                    if(exception[it]){
                        responsefw.andlist.push_back(startweek.startdate + boost::gregorian::date_duration(weeklength - it -1));
                    }
                }
            }
            //gestion des SAUF
            exception = getexceptpattern(firstweek.week_bs, startweek.week_bs);

            if(exception.any()){
                for(int it=0; it!= weeklength; it++) {
                    if(exception[it]){
                        responsefw.exceptlist.push_back(startweek.startdate + boost::gregorian::date_duration(weeklength - it -1));
                    }
                }
            }
            //gestion des periodes
            target& responsesw = target_map[startweekkey];
            responsefw.periodlist.front() = responsesw.periodlist.front();
            target_map.erase(startweekkey);
        }
        // la condition de service ne termine pas un dimanche, on va rabattre la borne supérieure sur la semaine précédente
        if((week_vector.size() > 2) && enddate.day_of_week() != boost::date_time::Sunday){
            week endweek = week_vector.back();
            long int endweekkey = endweek.week_bs.to_ulong();
            week lastweek = week_vector[week_vector.size() - 2];
            //on ne garde que la partie commune aux 2 semaines
            int weeklength = getnextmonday(enddate, -1);
            target& responselw = target_map[lastweek.week_bs.to_ulong()];
            //gestion des ET
            std::bitset<7> exception =getandpattern(lastweek.week_bs, endweek.week_bs);
            if(exception.any()){
                for(int it=6; it!= 6 + weeklength; it--) {
                    if(exception[it]){
                        responselw.andlist.push_back(endweek.startdate + boost::gregorian::date_duration(7 - it -1));
                    }
                }
            }
            //gestion des SAUF
            exception = getexceptpattern(lastweek.week_bs, endweek.week_bs);

            if(exception.any()){
                for(int it=6; it!= 6 + weeklength; it--) {
                    if(exception[it]){
                        responselw.exceptlist.push_back(endweek.startdate + boost::gregorian::date_duration(7 - it -1));
                    }
                }
            }
            //gestion des periodes
            target& responseew = target_map[endweekkey];
            responselw.periodlist.back() = responseew.periodlist.back();
            target_map.erase(endweekkey);
        }
    }
}

void MakeTranslation::translate(){
    int weekindice = -1;
    boost::gregorian::date_duration shift(6);
    for(std::vector<week>::iterator it=week_vector.begin(); it!= week_vector.end(); it++) {
        week weekit = *it;
        for(std::map<int, target>::iterator it=target_map.begin(); it!= target_map.end(); it++) {
            target& response = it->second;
            if (weekit.week_bs == response.week_bs){
                if (weekindice != -1){
                    if (weekit != week_vector.back()){
                        response.periodlist.back() = weekit.startdate + shift;
                    } else {
                        response.periodlist.back() = enddate;
                    }
                } else {
                    response.periodlist.push_back(weekit.startdate);
                    if (weekit != week_vector.back()){
                        response.periodlist.push_back(weekit.startdate + shift);
                    } else {
                        response.periodlist.push_back(enddate);
                    }
                }
                weekindice = 1;
            } else {
                weekindice = -1;
            }
        }
        weekindice = -1;
    }
}
