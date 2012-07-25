#include "vptranslator/vptranslator.h"

void vptranslator::initcibles(){
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
//    on recherche la position du premier lundi dans la chaine
    std::string substr;
    short int weeknumber = 0;
    //pos represente la position au sein de la condition de service
    size_t pos = getnextmonday(startdate, 1);
    size_t precpos = 0;
    boost::gregorian::date weekstartdate = startdate;

    while (precpos < CS.length()){
    //tant que le prochain lundi est inférieur à enddate, on découpe le régime
        if(pos < CS.length()){
            substr = CS.substr(precpos, pos - precpos);
        } else {
            substr = "0000000";
            substr.replace(0, CS.length() - precpos, CS.substr(precpos, CS.length() - precpos));
        };
        week_map[weeknumber].startdate = weekstartdate;
        //initialisation du bitset
        week_map[weeknumber].week_bs = std::bitset<7>(std::string(substr));
        std::map<int, target>::iterator it;
        it = target_map.find(week_map[weeknumber].week_bs.to_ulong());
        if (it != target_map.end()){
            it->second.count++;
            it->second.lastweeknumber = weeknumber;
        } else {
            target& response = target_map[week_map[weeknumber].week_bs.to_ulong()];
            response.week_bs = week_map[weeknumber].week_bs;
            response.firstweeknumber = weeknumber;
            response.used = false;
            response.count = 1;
        };

        weekstartdate = weekstartdate + boost::gregorian::date_duration(pos - precpos);
        weeknumber++;
        precpos = pos;
        pos = pos + 7;
    }
}

int MakeTranslation::getbesttarget(){
    short int maxcount = 0;
    int result= -1;

    for(std::map<int, target>::iterator it=target_map.begin(); it!= target_map.end(); it++) {
        if (!it->second.used && (it->second.count > maxcount)){
            result = it->first;
            maxcount = it->second.count;
        };
    }
    return result;
}

void MakeTranslation::bounddrawdown(){
    if(week_map.size() > 1){
        // la condition de service ne débute pas un lundi, on a rabattre la borne inférieure sur la semaine consécutive
        week startweek = week_map[0];
        long int startweekkey = startweek.week_bs.to_ulong();
        if (startweek.startdate.day_of_week() != boost::date_time::Monday){
        //on récupere la première semaine complète afin d'inclure la borne basse à cette semaine
            week firstweek = week_map[1];
           //on ne garde que la partie commune aux 2 semaines
            int weeklength = getnextmonday(startweek.startdate, 1);
            //gestion des ET
            std::bitset<7> exception = firstweek.week_bs;
            exception.flip() &= startweek.week_bs;
            target& responsefw = target_map[firstweek.week_bs.to_ulong()];

            if(exception.any()){
                for(int it=0; it!= weeklength; it++) {
                    if(exception[it]){
                        responsefw.andlist.push_back(startweek.startdate + boost::gregorian::date_duration(weeklength - it -1));
                    }
                }
            }
            //gestion des SAUF
            startweek.week_bs ^= firstweek.week_bs;
            exception &= startweek.week_bs;
            exception ^= startweek.week_bs;

            if(exception.any()){
                for(int it=0; it!= weeklength; it++) {
                    if(exception[it]){
                        responsefw.exceptlist.push_back(startweek.startdate + boost::gregorian::date_duration(it));
                    }
                }
            }
            //gestion des periodes
            target& responsesw = target_map[startweekkey];
            responsefw.periodlist[0] = responsesw.periodlist[0];
            std::cout << "target map avant" << target_map.size() << std::endl;
            target_map.erase(startweekkey);
            std::cout << "target map apres" << target_map.size() << std::endl;
        }
    }
}


void MakeTranslation::translate(){
    int weekindice = -1;
    int targetkey = getbesttarget();
    boost::gregorian::date_duration shift(6);
    while (targetkey >= 0){
        target& response = target_map[targetkey];
        for(std::map<int, week>::iterator it=week_map.begin(); it!= week_map.end(); it++) {
            if (it->second.week_bs.to_ulong() == response.week_bs.to_ulong()){
                if (weekindice != -1){
                    response.periodlist.erase(response.periodlist.end());
                    response.periodlist.push_back(it->second.startdate + shift);
                } else {
                    response.periodlist.push_back(it->second.startdate);
                    response.periodlist.push_back(it->second.startdate + shift);
                }
                weekindice = 1;
            } else {
                weekindice = -1;
            }
        }
        response.used = true;
        targetkey = getbesttarget();
        weekindice = -1;
   }
}
